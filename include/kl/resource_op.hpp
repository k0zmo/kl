#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/ctti.hpp"
#include "kl/resource.hpp"
#include "kl/resource_attributes.hpp"
#include "kl/serialization.hpp"
#include "kl/serialization_error.hpp"
#include "kl/utility.hpp"

#include <charconv>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace kl::resources {

struct access_context
{
    bool subtree_read_only = false;
};

template <typename T>
struct optional_traits
{
    static constexpr bool is_optional = false;
};

template <typename T>
struct optional_traits<std::optional<T>>
{
    static constexpr bool is_optional = true;
    using value_type = T;
};

namespace detail {

// Makes the ADL customization point name visible while remaining non-callable for (value, ctx).
void validate_resource();
void resource_key_matches();

namespace impl {

template <typename T, typename Context>
auto validate_candidate(T& value, Context& ctx, kl::priority_tag<2>)
    -> decltype(value.validate_resource(ctx), void())
{
    // Call member function
    value.validate_resource(ctx);
}

template <typename T, typename Context>
auto validate_candidate(T& value, Context& ctx, kl::priority_tag<1>)
    -> decltype(validate_resource(value, ctx), void())
{
    // Call via ADL
    validate_resource(value, ctx);
}

template <typename T, typename Context>
void validate_candidate(T&, Context&, kl::priority_tag<0>)
{
    // No .validate or validate_resource function defined
}

template <typename Element, typename Key>
auto key_matches(const Element& element, const Key& key, std::string_view segment,
                 kl::priority_tag<1>) -> decltype(resource_key_matches(element, key, segment))
{
    // Call via ADL
    return resource_key_matches(element, key, segment);
}

template <typename Element, typename Key>
bool key_matches(const Element&, const Key& key, std::string_view segment, kl::priority_tag<0>)
{
    // Default key matching implementation
    return key == segment;
}

} // namespace impl

template <typename T, typename Context>
void validate_candidate(T& value, Context& ctx)
{
    impl::validate_candidate(value, ctx, kl::priority_tag<2>{});
    if constexpr (optional_traits<T>::is_optional)
    {
        // We validate both optional<T> and T: the former can validate nullability,
        // while the latter keeps its own type invariants.
        if (value)
            validate_candidate(*value, ctx);
    }
}

template <typename Element, typename Key>
bool key_matches(const Element& element, const Key& key, std::string_view segment)
{
    return impl::key_matches(element, key, segment, kl::priority_tag<1>{});
}

template <typename Node, typename Element>
void check_duplicate_child_key(Node node, const Element& candidate)
{
    if constexpr (Node::template has_attribute<attributes::child_key_base_t>())
    {
        node.visit_attributes([&](const auto& attr) {
            using attribute_type = std::decay_t<decltype(attr)>;
            if constexpr (attributes::detail::is_child_key_v<attribute_type>)
            {
                for (const auto& element : node.value())
                {
                    if (element.*attribute_type::ptr == candidate.*attribute_type::ptr)
                        throw duplicate_child_key_error{};
                }
            }
        });
    }
}

template <typename Field>
struct field_node
{
    Field field_;

    decltype(auto) value() const noexcept(noexcept(field_.value()))
    {
        return field_.value();
    }

    template <typename Attribute>
    static constexpr bool has_attribute() noexcept
    {
        return Field::template has<Attribute>();
    }

    template <typename Visitor>
    decltype(auto) visit_attributes(Visitor&& visitor) const
    {
        return field_.visit_attributes(std::forward<Visitor>(visitor));
    }
};

template <typename T>
struct root_node
{
    T& value_;

    decltype(auto) value() const noexcept
    {
        return value_;
    }

    template <typename Attribute>
    static constexpr bool has_attribute() noexcept
    {
        // root node has no field attributes
        return false;
    }
};

template <typename T>
struct element_node
{
    T& value_;

    decltype(auto) value() const noexcept
    {
        return value_;
    }

    template <typename Attribute>
    static constexpr bool has_attribute() noexcept
    {
        // Collection elements have no field attributes of their own.
        return false;
    }
};

// We don't want to classify string as path-traversable
template <typename T>
struct is_string_like : std::false_type {};

template <typename Char, typename Traits, typename Allocator>
struct is_string_like<std::basic_string<Char, Traits, Allocator>> : std::true_type {};

template <typename Char, typename Traits>
struct is_string_like<std::basic_string_view<Char, Traits>> : std::true_type {};

template <typename T>
inline constexpr bool is_index_traversable_v =
    kl::detail::has_at_v<T> && !is_string_like<std::decay_t<T>>::value;

template <typename T>
inline constexpr bool is_key_traversable_v =
   kl::detail::is_range<T>::value && !is_string_like<std::decay_t<T>>::value;

inline bool parse_index(std::string_view text, std::size_t& out)
{
    if (text.empty())
        return false;

    std::size_t value = 0;
    const auto* first = text.data();
    const auto* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last)
        return false;

    out = value;
    return true;
}

template <typename Result, typename Node, typename Visitor>
Result visit_node(Node node, path_view path, access_context& ctx, Visitor&& visitor)
{
    if (path.empty())
        return std::forward<Visitor>(visitor)(node, ctx);

    using value_type = std::decay_t<decltype(node.value())>;

    if constexpr (kl::ctti::is_reflectable_v<value_type>)
    {
        // We can't use optional here as it trips over with rapidjson types, at least with MSVC's STL
        std::unique_ptr<Result> result;

        ctti::reflect_object(node.value(), [&](auto field) {
            if (result)
                return;

            // FIXME: Use canonical resource name here.
            // Could be field.name(), or serialized_name(field) if rename() should affect paths.
            if (path.front() != field.name())
                return;

            const auto subtree_read_only_before = ctx.subtree_read_only;
            ctx.subtree_read_only = ctx.subtree_read_only ||
                                    Node::template has_attribute<attributes::subtree_read_only_t>();

            result = std::make_unique<Result>(
                visit_node<Result>(field_node<decltype(field)>{field},
                                   path.drop_front(),
                                   ctx,
                                   std::forward<Visitor>(visitor)));

            ctx.subtree_read_only = subtree_read_only_before;
        }); 

        if (result)
            return std::move(*result);

        throw path_segment_not_found_error {};
    }
    else if constexpr (Node::template has_attribute<attributes::child_key_base_t>() &&
                       is_key_traversable_v<value_type>)
    {
        access_context child_ctx = ctx;
        child_ctx.subtree_read_only =
            child_ctx.subtree_read_only ||
            Node::template has_attribute<attributes::subtree_read_only_t>();

        std::unique_ptr<Result> result;
        node.visit_attributes([&](const auto& attr) {
            if (result)
                return;

            using attribute_type = std::decay_t<decltype(attr)>;
            if constexpr (attributes::detail::is_child_key_v<attribute_type>)
            {
                for (auto& element : node.value())
                {
                    if (!key_matches(element, element.*attribute_type::ptr, path.front()))
                        continue;

                    result = std::make_unique<Result>(
                        visit_node<Result>(
                            element_node<std::remove_reference_t<decltype(element)>>{element},
                            path.drop_front(),
                            child_ctx,
                            std::forward<Visitor>(visitor)));
                    return;
                }
            }
        });

        if (result)
            return std::move(*result);

        throw path_segment_not_found_error{};
    }
    else if constexpr (is_index_traversable_v<value_type>)
    {
        std::size_t index = 0;
        if (!parse_index(path.front(), index))
            throw path_segment_not_found_error{};

        access_context child_ctx = ctx;
        child_ctx.subtree_read_only =
            child_ctx.subtree_read_only ||
            Node::template has_attribute<attributes::subtree_read_only_t>();

        auto& element = [&]() -> decltype(auto) {
            try
            {
                return node.value().at(index);
            }
            catch (const std::out_of_range&)
            {
                throw path_segment_not_found_error{};
            }
        }();

        return visit_node<Result>(
            element_node<std::remove_reference_t<decltype(element)>>{element},
            path.drop_front(),
            child_ctx,
            std::forward<Visitor>(visitor));
    }
    else
    {
        // The remaining path cannot be traversed.
        throw path_not_traversable_error{};
    }
}

} // namespace detail

template <typename Result, typename T, typename Visitor>
Result visit_at_path(T& object, path_view path, Visitor&& visitor)
{
    access_context ctx;
    return detail::visit_node<Result>(detail::root_node<T>{object}, path, ctx,
                                      std::forward<Visitor>(visitor));
}

template <typename T, typename Context>
auto dump_at_path(const T& obj, path_view path, Context& ctx)
    -> decltype(kl::serialization::dump(obj, ctx))
{
    using return_type = decltype(kl::serialization::dump(obj, ctx));
    return kl::resources::visit_at_path<return_type>(
        obj, path, [&](auto field, [[maybe_unused]] auto& access_ctx) {
            return kl::serialization::dump(field.value(), ctx);
        });
}

template <typename T, typename Context>
auto serialize_at_path(const T& obj, path_view path, Context& ctx)
    -> decltype(kl::serialization::serialize(obj, ctx))
{
    using return_type = decltype(kl::serialization::serialize(obj, ctx));
    return kl::resources::visit_at_path<return_type>(
        obj, path, [&](auto field, [[maybe_unused]] auto& access_ctx) {
            return kl::serialization::serialize(field.value(), ctx);
        });
}

template <typename T, typename Value, typename Context>
void deserialize_at_path(T& obj, path_view path, const Value& value, Context& ctx)
{
    (void)kl::resources::visit_at_path<int>(
        obj, path, [&](auto field, [[maybe_unused]] auto& access_ctx) {
            kl::serialization::deserialize(field.value(), value, ctx);
            return 1; // Dummy value, doesn't matter
        });
}

// HTTP-like verbs for kl::resources::resource

// Based on HTTP status codes so it's easy to map one to another
enum class status_code
{
    ok = 200,
    created = 201,
    no_content = 204,
    bad_request = 400,
    not_found = 404,
    method_not_allowed = 405,
    conflict = 409
};

struct operation_result
{
    status_code status;
    std::string body;
};

template <typename T, typename Dumper>
operation_result get(const resource<T>& r, path_view path, Dumper&& dumper)
try
{
    return kl::resources::visit_at_path<operation_result>(
        r.value, path, [&](auto node, [[maybe_unused]] auto& access_ctx) {
            return operation_result{status_code::ok, dumper(node.value())};
        });
}
catch (const path_segment_not_found_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error&)
{
    return operation_result{status_code::not_found, {}};
}

template <typename T, typename Value, typename Context>
operation_result put(resource<T>& r, path_view path, const Value& value, Context& ctx)
try
{
    auto result = kl::resources::visit_at_path<operation_result>(
        r.value, path, [&](auto node, auto& access_ctx) {
            constexpr bool is_read_only =
                decltype(node)::template has_attribute<attributes::read_only_t>();
            if (is_read_only || access_ctx.subtree_read_only)
                return operation_result{status_code::method_not_allowed, {}};

            using node_value_type = std::decay_t<decltype(node.value())>;

            auto candidate = node.value();
            kl::serialization::deserialize(candidate, value, ctx);
            detail::validate_candidate(candidate, ctx);
            node.value() = std::move(candidate);

            r.state.modifications.notify_changed(
                path, kl::ctti::is_reflectable_v<node_value_type>);

            return operation_result{status_code::ok, {}};
        });

    return result;
}
catch (const path_segment_not_found_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const validation_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}
catch (const kl::serialization::deserialize_error&)
{
    return operation_result{status_code::bad_request, {}};
}

template <typename T, typename Value, typename Context>
operation_result post(resource<T>& r, path_view path, const Value& value, Context& ctx)
try
{
    auto result = kl::resources::visit_at_path<operation_result>(
        r.value, path, [&](auto node, auto& access_ctx) {
            constexpr bool is_read_only =
                decltype(node)::template has_attribute<attributes::read_only_t>();
            if (is_read_only || access_ctx.subtree_read_only)
                return operation_result{status_code::method_not_allowed, {}};

            using collection_type = std::decay_t<decltype(node.value())>;
            if constexpr (kl::detail::is_growable_range<collection_type>::value)
            {
                using element_type = typename collection_type::value_type;

                element_type candidate{};
                kl::serialization::deserialize(candidate, value, ctx);
                detail::check_duplicate_child_key(node, candidate);
                detail::validate_candidate(candidate, ctx);
                node.value().push_back(std::move(candidate));
                r.state.modifications.notify_changed(path, true);

                return operation_result{status_code::created, {}};
            }
            else
            {
                return operation_result{status_code::method_not_allowed, {}};
            }
        });

    return result;
}
catch (const path_segment_not_found_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const duplicate_child_key_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}
catch (const validation_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}
catch (const kl::serialization::deserialize_error&)
{
    return operation_result{status_code::bad_request, {}};
}

template <typename T, typename Context>
operation_result del(resource<T>& r, path_view path, Context& ctx)
try
{
    auto result = kl::resources::visit_at_path<operation_result>(
        r.value, path, [&](auto node, auto& access_ctx) {
            constexpr bool is_read_only =
                decltype(node)::template has_attribute<attributes::read_only_t>();
            if (is_read_only || access_ctx.subtree_read_only)
                return operation_result{status_code::method_not_allowed, {}};

            using node_value_type = std::decay_t<decltype(node.value())>;
            using optional_traits = kl::resources::optional_traits<node_value_type>;

            if constexpr (optional_traits::is_optional)
            {
                auto candidate = node.value();
                // Allow T (not optional<T>) to veto deletion
                if (candidate)
                    detail::validate_candidate(*candidate, ctx);
                candidate.reset();
                // This is most likely no-op since candidate is optional<T>, not T
                detail::validate_candidate(candidate, ctx);
                node.value() = std::move(candidate);

                r.state.modifications.notify_changed(
                    path, kl::ctti::is_reflectable_v<typename optional_traits::value_type>);

                return operation_result{status_code::no_content, {}};
            }
            else
            {
                return operation_result{status_code::method_not_allowed, {}};
            }
        });

    return result;
}
catch (const path_segment_not_found_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error&)
{
    return operation_result{status_code::not_found, {}};
}
catch (const validation_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}

} // namespace kl::resources
