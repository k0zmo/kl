#pragma once

#include "kl/utility.hpp"
#include "kl/ctti.hpp"

#include <exception>
#include <gsl/span>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace kl::resources {

namespace attributes {

struct read_only_t {};
struct subtree_read_only_t {};
struct id_field_t {};

inline constexpr read_only_t read_only{};
inline constexpr subtree_read_only_t subtree_read_only{};
inline constexpr id_field_t id_field{};

} // namespace attributes

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

namespace detail {

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

template <typename Result, typename Node, typename Visitor>
Result visit_node(Node node, path_view path, Visitor&& visitor)
{
    if (path.empty())
        return visitor(node);

    using value_type = std::decay_t<decltype(node.value())>;

    if constexpr (!kl::ctti::is_reflectable_v<value_type>)
    {
        // Leaf values are valid targets, but the remaining path cannot be traversed.
        throw path_not_traversable_error{};
    }
    else
    {
        std::optional<Result> result;

        ctti::reflect_object(node.value(), [&](auto field) {
            if (result)
                return;

            // FIXME: Use canonical resource name here.
            // Could be field.name(), or serialized_name(field) if rename() should affect paths.
            if (path.front() != field.name())
                return;

            result = visit_node<Result>(field_node<decltype(field)>{field},
                                        path.drop_front(),
                                        std::forward<Visitor>(visitor));
        }); 

        if (result)
            return std::move(*result);

        throw path_segment_not_found_error {};
    }
}

}

template <typename Result, typename T, typename Visitor>
Result visit_at_path(T& object, path_view path, Visitor&& visitor)
{
    return detail::visit_node<Result>(detail::root_node<T>{object},
                                      path,
                                      std::forward<Visitor>(visitor));
}

} // namespace kl::resources
