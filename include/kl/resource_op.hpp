#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/ctti.hpp"
#include "kl/resource.hpp"
#include "kl/resource_attributes.hpp"
#include "kl/serialization.hpp"
#include "kl/serialization_error.hpp"
#include "kl/utility.hpp"

#include <cassert>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace kl::resources {

struct access_context
{
    bool subtree_read_only = false;
    bool node_read_only = false;

    template <typename Node>
    bool is_node_read_only() const noexcept
    {
        constexpr bool is_read_only =
            Node::template has_attribute<attributes::read_only_t>();
        return is_read_only || subtree_read_only || node_read_only;
    }

    template <typename Node>
    bool is_child_node_read_only() const noexcept
    {
        return is_node_read_only<Node>() ||
               Node::template has_attribute<attributes::subtree_read_only_t>();
    }

    template <typename Node>
    access_context update(const Node&) noexcept
    {
        access_context copy = *this;
        subtree_read_only = subtree_read_only ||
            Node::template has_attribute<attributes::subtree_read_only_t>();
        return copy;
    }

    template <typename Node, typename Field>
    access_context update(const Node& node, const Field& field) noexcept
    {
        auto copy = update(node);
        node_read_only = node_read_only ||
            node.is_immutable_child(field.value());
        return copy;
    }
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

template <typename T, typename Context>
void validate_self(T& value, Context& ctx)
{
    impl::validate_candidate(value, ctx, kl::priority_tag<2>{});
}

template <typename Element, typename Key>
bool key_matches(const Element& element, const Key& key, std::string_view segment)
{
    return impl::key_matches(element, key, segment, kl::priority_tag<1>{});
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

    template <typename Attribute, typename Visitor>
    void visit_attributes(Visitor&& visitor) const
    {
        field_.template visit_attributes<Attribute>(std::forward<Visitor>(visitor));
    }

    template <typename Child>
    bool is_immutable_child(const Child&) const noexcept
    {
        return false;
    }

    template <typename Candidate>
    bool preserves_identity(const Candidate&) const noexcept
    {
        return true;
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

    template <typename Child>
    bool is_immutable_child(const Child&) const noexcept
    {
        return false;
    }

    template <typename Candidate>
    bool preserves_identity(const Candidate&) const noexcept
    {
        return true;
    }
};

template <typename T, auto ChildKeyPtr = nullptr>
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

    // Checks whether a given child is a key (attribute child_key points to it)
    // that is immutable to changes
    template <typename Child>
    bool is_immutable_child(const Child& child) const noexcept
    {
        if constexpr (std::is_same_v<decltype(ChildKeyPtr), std::nullptr_t>)
            return false;
        else if constexpr (std::is_same_v<std::decay_t<Child>,
                                          std::decay_t<decltype(value_.*ChildKeyPtr)>>)
            return std::addressof(child) == std::addressof(value_.*ChildKeyPtr);
        else
            return false;
    }

    // Checks whether a new candidate (with possible new key) has the same key value
    template <typename Candidate>
    bool preserves_identity(const Candidate& candidate) const
    {
        if constexpr (std::is_same_v<decltype(ChildKeyPtr), std::nullptr_t>)
            return true;
        else
            return value_.*ChildKeyPtr == candidate.*ChildKeyPtr;
    }
};

// We don't want to classify string as path-traversable
template <typename T>
struct is_string_like : std::false_type {};

template <typename Char, typename Traits, typename Allocator>
struct is_string_like<std::basic_string<Char, Traits, Allocator>> : std::true_type {};

template <typename Char, typename Traits>
struct is_string_like<std::basic_string_view<Char, Traits>> : std::true_type {};

// A type trait telling whether T can be traversed using an index via .at()
template <typename T>
inline constexpr bool is_index_traversable_v =
    kl::detail::has_at_v<T> && !is_string_like<std::decay_t<T>>::value;

// A type trait for all ranges excluding a string-like types
template <typename T>
inline constexpr bool is_non_string_range_v =
   kl::detail::is_range<T>::value && !is_string_like<std::decay_t<T>>::value;

// A type trait telling whether T can be traversed using a key from subsequent path segment
template <typename T>
inline constexpr bool is_key_traversable_v = is_non_string_range_v<T>;

// A concept whether a T can be appended to using POST method and, if validation fails afterward,
// restore the previous collection state
template <typename T>
inline constexpr bool is_postable_range_v =
    is_non_string_range_v<T> &&
    kl::detail::has_value_type_v<T> &&
    kl::detail::has_push_back_v<T> &&
    kl::detail::has_pop_back_v<T>; // Remove the just added element if validation fails

// A concept for T whether we can erase an element and, if validation fails afterward, restore the
// previous collection state
template <typename T>
inline constexpr bool is_deletable_range_v =
    is_non_string_range_v<T> &&
    kl::detail::has_erase_v<T> &&
    std::is_copy_constructible_v<T> && // Take a snapshot before erase
    std::is_move_assignable_v<T>; // Restore from snapshot if validation fails

template <typename T, typename Context>
void validate_tree(T& value, Context& ctx)
{
    validate_self(value, ctx);

    using value_type = std::decay_t<T>;
    if constexpr (optional_traits<value_type>::is_optional)
    {
        if (value)
            validate_tree(*value, ctx);
    }
    else if constexpr (kl::ctti::is_reflectable_v<value_type>)
    {
        ctti::reflect_object(value, [&](auto field) {
            validate_tree(field.value(), ctx);
        });
    }
    else if constexpr (is_non_string_range_v<value_type>)
    {
        for (auto& element : value)
            validate_tree(element, ctx);
    }
}

inline constexpr struct copy_backup_t {} copy_backup{};
inline constexpr struct move_if_noexcept_backup_t {} move_if_noexcept_backup{};

template <typename T>
class rollback_value
{
public:
    rollback_value(T& value, copy_backup_t)
        : value_{value}, old_value_{value}
    {
    }

    rollback_value(T& value, move_if_noexcept_backup_t)
        : value_{value}, old_value_{std::move_if_noexcept(value)}
    {
    }

    rollback_value(const rollback_value&) = delete;
    rollback_value& operator=(const rollback_value&) = delete;

    ~rollback_value() noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (!committed_)
            value_ = std::move(old_value_);
    }

    void commit() noexcept
    {
        committed_ = true;
    }

private:
    T& value_;
    T old_value_;
    bool committed_ = false;
};

template <typename T>
class pop_back_rollback
{
public:
    explicit pop_back_rollback(T& value) noexcept
        : value_{value}
    {
    }

    pop_back_rollback(const pop_back_rollback&) = delete;
    pop_back_rollback& operator=(const pop_back_rollback&) = delete;

    ~pop_back_rollback() noexcept(noexcept(std::declval<T&>().pop_back()))
    {
        if (!committed_)
            value_.pop_back();
    }

    void commit() noexcept
    {
        committed_ = true;
    }

private:
    T& value_;
    bool committed_ = false;
};

template <typename Node, typename Element>
void check_duplicate_child_key(Node node, const Element& candidate)
{
    node.template visit_attributes<attributes::child_key_base_t>([&](const auto& attr) {
        using attribute_type = std::decay_t<decltype(attr)>;
        for (const auto& element : node.value())
        {
            if (element.*attribute_type::ptr == candidate.*attribute_type::ptr)
                throw duplicate_child_key_error{};
        }
    });
}

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
Result visit_node(Node node, path_view path, std::size_t stop_remaining,
                  access_context& ctx, Visitor&& visitor)
{
    if (path.size() == stop_remaining)
        return std::forward<Visitor>(visitor)(node, path, ctx);

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

            const auto ctx_before = ctx.update(node, field);
            result = std::make_unique<Result>(
                visit_node<Result>(field_node<decltype(field)>{field},
                                   path.drop_front(),
                                   stop_remaining,
                                   ctx,
                                   std::forward<Visitor>(visitor)));
            ctx = ctx_before;
        }); 

        if (result)
            return std::move(*result);

        throw path_segment_not_found_error {};
    }
    else if constexpr (Node::template has_attribute<attributes::child_key_base_t>() &&
                       is_key_traversable_v<value_type>)
    {
        access_context child_ctx = ctx.update(node);

        std::unique_ptr<Result> result;
        node.template visit_attributes<attributes::child_key_base_t>([&](const auto& attr) {
            if (result)
                return;

            using attribute_type = std::decay_t<decltype(attr)>;
            for (auto& element : node.value())
            {
                if (!key_matches(element, element.*attribute_type::ptr, path.front()))
                    continue;

                result = std::make_unique<Result>(
                    visit_node<Result>(
                        element_node<std::remove_reference_t<decltype(element)>,
                                     attribute_type::ptr>{element},
                        path.drop_front(),
                        stop_remaining,
                        child_ctx,
                        std::forward<Visitor>(visitor)));
                return;
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

        access_context child_ctx = ctx.update(node);

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
            stop_remaining,
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
Result visit_at_path(T& object, path_view path, std::size_t stop_remaining,
                     Visitor&& visitor)
{
    access_context ctx;
    return detail::visit_node<Result>(detail::root_node<T>{object}, path,
                                      stop_remaining, ctx,
                                      std::forward<Visitor>(visitor));
}

template <typename Result, typename T, typename Visitor>
Result visit_at_path(T& object, path_view path, Visitor&& visitor)
{
    return kl::resources::visit_at_path<Result>(
        object, path, 0,
        [&](auto node, [[maybe_unused]] path_view remaining, auto& ctx) {
            return std::forward<Visitor>(visitor)(node, ctx);
        });
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

namespace detail {

template <typename Root, typename Node, typename Context>
bool delete_direct_child(resource<Root>& r, Node node, path_view full_path,
                         path_view remaining, access_context& access_ctx, Context& ctx,
                         operation_result& out)
{
    assert(remaining.size() == 1);

    using value_type = std::decay_t<decltype(node.value())>;
    auto& collection = node.value();

    if constexpr (Node::template has_attribute<attributes::child_key_base_t>() &&
                       is_key_traversable_v<value_type>)
    {
        bool found = false;
        node.template visit_attributes<attributes::child_key_base_t>([&](const auto& attr) {
            if (found)
                return;

            for (auto it = collection.begin(); it != collection.end(); ++it)
            {
                using attribute_type = std::decay_t<decltype(attr)>;
                if (!key_matches(*it, (*it).*attribute_type::ptr, remaining.front()))
                    continue;

                found = true;
                if (access_ctx.is_child_node_read_only<Node>())
                {
                    out = operation_result{status_code::method_not_allowed, {}};
                }
                else
                {
                    if constexpr (is_deletable_range_v<value_type>)
                    {
                        rollback_value rollback{collection, copy_backup};
                        collection.erase(it);
                        validate_tree(r.value, ctx);
                        rollback.commit();

                        r.state.modifications.notify_changed(full_path.drop_back(), true);
                        out = operation_result{status_code::no_content, {}};
                    }
                    else
                        out = operation_result{status_code::method_not_allowed, {}};
                }
                return;
            }
        });

        if (!found)
            throw path_segment_not_found_error{};

        return true;
    }
    else if constexpr (is_index_traversable_v<value_type>)
    {
        std::size_t index = 0;
        if (!parse_index(remaining.front(), index))
            throw path_segment_not_found_error{};

        if (access_ctx.is_child_node_read_only<Node>())
        {
            out = operation_result{status_code::method_not_allowed, {}};
        }
        else
        {
            if constexpr (is_deletable_range_v<value_type>)
            {
                try
                {
                    // Use .at to do the bound check
                    (void)collection.at(index);
                }
                catch (const std::out_of_range&)
                {
                    throw path_segment_not_found_error{};
                }
                auto it = collection.begin();
                std::advance(it, index);

                rollback_value rollback{collection, copy_backup};
                collection.erase(it);
                validate_tree(r.value, ctx);
                rollback.commit();

                r.state.modifications.notify_changed(full_path.drop_back(), true);
                out = operation_result{status_code::no_content, {}};
            }
            else
            {
                out = operation_result{status_code::method_not_allowed, {}};
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

} // namespace detail

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
        r.value, path, [&](auto node, access_context& access_ctx) {
            if (access_ctx.is_node_read_only<decltype(node)>())
                return operation_result{status_code::method_not_allowed, {}};

            using node_value_type = std::decay_t<decltype(node.value())>;

            auto candidate = node.value();
            kl::serialization::deserialize(candidate, value, ctx);
            if (!node.preserves_identity(candidate))
                return operation_result{status_code::method_not_allowed, {}};

            detail::rollback_value rollback{
                node.value(), detail::move_if_noexcept_backup};
            node.value() = std::move(candidate);
            detail::validate_tree(r.value, ctx);
            rollback.commit();

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
        r.value, path, [&](auto node, access_context& access_ctx) {
            if (access_ctx.is_child_node_read_only<decltype(node)>())
                return operation_result{status_code::method_not_allowed, {}};

            using collection_type = std::decay_t<decltype(node.value())>;
            if constexpr (detail::is_postable_range_v<collection_type>)
            {
                using element_type = typename collection_type::value_type;

                element_type candidate{};
                kl::serialization::deserialize(candidate, value, ctx);
                detail::check_duplicate_child_key(node, candidate);
                detail::validate_candidate(candidate, ctx);
                node.value().push_back(std::move(candidate));
                detail::pop_back_rollback rollback{node.value()};
                detail::validate_tree(r.value, ctx);
                rollback.commit();
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
    if (!path.empty())
    {
        operation_result direct_result{};
        const bool handled = kl::resources::visit_at_path<bool>(
            r.value, path, 1, [&](auto node, path_view remaining, auto& access_ctx) {
                return detail::delete_direct_child(
                    r, node, path, remaining, access_ctx, ctx, direct_result);
            });

        if (handled)
            return direct_result;
    }

    return kl::resources::visit_at_path<operation_result>(
        r.value, path, [&](auto node, access_context& access_ctx) {
            if (access_ctx.is_node_read_only<decltype(node)>())
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
                detail::rollback_value rollback{
                    node.value(), detail::move_if_noexcept_backup};
                node.value() = std::move(candidate);
                detail::validate_tree(r.value, ctx);
                rollback.commit();

                r.state.modifications.notify_changed(
                    path, kl::ctti::is_reflectable_v<typename optional_traits::value_type>);

                return operation_result{status_code::no_content, {}};
            }
            else
            {
                return operation_result{status_code::method_not_allowed, {}};
            }
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
catch (const validation_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}

} // namespace kl::resources
