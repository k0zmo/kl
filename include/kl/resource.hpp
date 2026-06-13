#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/serialization.hpp"
#include "kl/serialization_error.hpp"
#include "kl/type_traits.hpp"
#include "kl/utility.hpp"
#include "kl/ctti.hpp"

#include <exception>

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace kl::resources {

namespace attributes {

struct read_only_t {};
struct subtree_read_only_t {};

namespace detail {

struct child_key_base_t {};

} // namespace detail

template <auto Ptr>
struct child_key_t : detail::child_key_base_t
{
    static constexpr auto ptr = Ptr;
};

namespace detail {

template <typename T>
struct is_child_key : std::false_type {};

template <auto Ptr>
struct is_child_key<child_key_t<Ptr>> : std::true_type {};

template <typename T>
inline constexpr bool is_child_key_v = is_child_key<T>::value;

} // namespace detail

// Marks the field as immutable (but not its descendants)
inline constexpr read_only_t read_only{};

// Marks all the child fields as immutable (but not the field itself)
inline constexpr subtree_read_only_t subtree_read_only{};

// Traverses a collection field by matching this member value instead of by index.
template <auto Ptr>
inline constexpr child_key_t<Ptr> child_key{};

} // namespace attributes

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

struct access_context
{
    bool subtree_read_only = false;
};

// FIXME: Use something more sophisticated
class path_view
{
public:
    constexpr path_view() = default;

    constexpr path_view(std::initializer_list<std::string_view> path) noexcept
        : data_{path.begin()}, size_{path.size()}
    {
    }

    bool empty() const noexcept { return size() == 0; }
    std::size_t size() const noexcept { return size_; }
    std::string_view operator[](std::size_t i) const noexcept { return data_[i]; }
    std::string_view front() const noexcept { return data_[0]; }
    path_view drop_front() const noexcept { return path_view{data_ + 1, size_ - 1}; }

private:
    constexpr path_view(const std::string_view* data, std::size_t size) noexcept
        : data_{data}, size_{size}
    {
    }

private:
    const std::string_view* data_ = nullptr;
    std::size_t size_ = 0;
};

inline std::string canonical(path_view path, std::size_t count)
{
    if (count == 0)
        return "/";

    std::string out;
    for (std::size_t i = 0U; i < count; ++i)
    {
        out += "/";
        out += path[i];
    }

    return out;
}

inline std::string canonical(path_view path)
{
    return canonical(path, path.size());
}

template <typename Map, typename Generation = typename Map::mapped_type>
Generation lookup(const Map& map, const std::string& key)
{
    auto it = map.find(key);
    return it != map.end() ? it->second : Generation{0};
}

class modification_tracker
{
public:
    enum class generation : std::uint64_t {};
    
    generation current() const noexcept { return current_;}

    generation notify_changed(path_view path, bool subtree = false)
    {
        auto gen = generation{underlying_cast(current_) + 1};
        current_ = gen;

        // Mark the node and all ancestors, so parent watchers observe child changes
        for (std::size_t i = 0; i <= path.size(); ++i)
            node_generation_[canonical(path, i)] = gen;

        // If a whole subtree was replaced, descendants should also appear changed, without walking
        // every reflected child.
        if (subtree)
            subtree_generation_[canonical(path)] = gen;

        return gen;
    }

    generation changed_at(path_view path) const
    {
        auto result = lookup(node_generation_, canonical(path));

        // If any ancestor subtree was replaced, this path changed too.
        for (std::size_t i = 0U; i <= path.size(); ++i)
        {
            auto sub = lookup(subtree_generation_, canonical(path, i));

            if (underlying_cast(sub) > underlying_cast(result))
                result = sub;
        }

        return result;
    }

    bool changed_since(path_view path, generation old) const
    {
        return changed_at(path) != old;
    }

private:
    generation current_{1};
    std::unordered_map<std::string, generation> node_generation_;
    std::unordered_map<std::string, generation> subtree_generation_;
};

struct resource_state
{
    modification_tracker modifications;
};

template <typename T>
struct resource
{
    T value;
    resource_state state;
};

template <typename T>
resource<T> make_resource(T value)
{
    return {std::move(value), {}};
}

struct resource_error : std::exception
{
    const char* what() const noexcept override
    {
        return "resource error";
    }
};

struct path_not_traversable_error : resource_error
{
    const char* what() const noexcept override
    {
        return "path continues through a non-object value";
    }
};

struct path_segment_not_found_error : resource_error
{
    const char* what() const noexcept override
    {
        return "path segment not found";
    }
};

struct validation_error : resource_error
{
    explicit validation_error(const char* message)
        : validation_error(std::string(message))
    {
    }

    explicit validation_error(std::string message) noexcept
        : message_(std::move(message))
    {
    }

    const char* what() const noexcept override
    {
        return message_.c_str();
    }

private:
    std::string message_;
};

namespace detail {

// Makes the ADL customization point name visible while remaining non-callable for (value, ctx).
void validate_resource();

template <typename T, typename Context>
auto validate_candidate_impl(T& value, Context& ctx, ::kl::priority_tag<2>)
    -> decltype(value.validate_resource(ctx), void())
{
    value.validate_resource(ctx);
}

template <typename T, typename Context>
auto validate_candidate_impl(T& value, Context& ctx, ::kl::priority_tag<1>)
    -> decltype(validate_resource(value, ctx), void())
{
    validate_resource(value, ctx);
}

template <typename T, typename Context>
void validate_candidate_impl(T&, Context&, ::kl::priority_tag<0>)
{
    // No .validate or validate_resource function defined
}

template <typename T, typename Context>
void validate_candidate(T& value, Context& ctx)
{
    validate_candidate_impl(value, ctx, ::kl::priority_tag<2>{});
    if constexpr (optional_traits<T>::is_optional)
    {
        // We validate both optional<T> and T: the former can validate nullability,
        // while the latter keeps its own type invariants.
        if (value)
            validate_candidate(*value, ctx);
    }
}

template <typename Field>
struct field_node
{
    Field field_;

    decltype(auto) value() const noexcept
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

template <typename T>
struct is_string_like : std::false_type {};

template <typename Char, typename Traits, typename Allocator>
struct is_string_like<std::basic_string<Char, Traits, Allocator>> : std::true_type
{
};

template <typename Char, typename Traits>
struct is_string_like<std::basic_string_view<Char, Traits>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_index_traversable_v =
    ::kl::detail::has_at_v<T> && !is_string_like<std::decay_t<T>>::value;

template <typename T>
inline constexpr bool is_key_traversable_v =
    ::kl::detail::is_range<T>::value && !is_string_like<std::decay_t<T>>::value;

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

// Makes the ADL customization point name visible while remaining non-callable for
// (element, key, segment).
void resource_key_matches();

template <typename Element, typename Key>
auto key_matches_impl(const Element& element, const Key& key, std::string_view segment,
                      ::kl::priority_tag<1>)
    -> decltype(resource_key_matches(element, key, segment))
{
    return resource_key_matches(element, key, segment);
}

template <typename Element, typename Key>
bool key_matches_impl(const Element&, const Key& key, std::string_view segment,
                      ::kl::priority_tag<0>)
{
    return key == segment;
}

template <typename Element, typename Key>
bool key_matches(const Element& element, const Key& key, std::string_view segment)
{
    return key_matches_impl(element, key, segment, ::kl::priority_tag<1>{});
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
    else if constexpr (Node::template has_attribute<attributes::detail::child_key_base_t>() &&
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
catch (const path_segment_not_found_error& ex)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error& ex)
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
catch (const path_segment_not_found_error& ex)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error& ex)
{
    return operation_result{status_code::not_found, {}};
}
catch (const validation_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}
catch (const kl::serialization::deserialize_error& ex)
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
catch (const path_segment_not_found_error& ex)
{
    return operation_result{status_code::not_found, {}};
}
catch (const path_not_traversable_error& ex)
{
    return operation_result{status_code::not_found, {}};
}
catch (const validation_error& ex)
{
    return operation_result{status_code::conflict, ex.what()};
}

} // namespace kl::resources
