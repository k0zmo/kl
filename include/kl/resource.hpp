#pragma once

#include "kl/path.hpp"
#include "kl/utility.hpp"

#include <exception>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace kl::resources {

class modification_tracker
{
public:
    enum class generation : std::uint64_t {};

    generation current() const noexcept { return current_;}

    generation notify_changed(const kl::path& path, bool subtree = false)
    {
        return notify_changed(path.view(), subtree);
    }

    generation notify_changed(path_view path, bool subtree = false)
    {
        auto gen = generation{underlying_cast(current_) + 1};
        current_ = gen;

        // Mark the node and all ancestors, so parent watchers observe child changes
        for (std::size_t i = 0; i <= path.size(); ++i)
            assign(node_generation_, path.prefix(i), gen);

        // If a whole subtree was replaced, descendants should also appear changed,
        // without walking every reflected child.
        if (subtree)
            assign(subtree_generation_, path.prefix(path.size()), gen);

        return gen;
    }

    generation changed_at(const kl::path& path) const
    {
        return changed_at(path.view());
    }

    generation changed_at(path_view path) const
    {
        auto result = generation{0};

        // If any ancestor subtree was replaced, this path changed too.
        for (std::size_t i = 0U; i <= path.size(); ++i)
        {
            const auto key = path.prefix(i);

            if (i == path.size())
            {
                const auto node = lookup(node_generation_, key);
                if (underlying_cast(node) > underlying_cast(result))
                    result = node;
            }

            const auto sub = lookup(subtree_generation_, key);

            if (underlying_cast(sub) > underlying_cast(result))
                result = sub;
        }

        return result;
    }

    bool changed_since(const kl::path& path, generation old) const
    {
        return changed_since(path.view(), old);
    }

    bool changed_since(path_view path, generation old) const
    {
        return underlying_cast(changed_at(path)) > underlying_cast(old);
    }

private:
    template <typename Map, typename Generation = typename Map::mapped_type>
    static void assign(Map& map, std::string_view key, Generation value)
    {
        // Assign (no key allocation) or insert (need to allocate a key as string)
        auto it = map.find(key);
        if (it != map.end())
        {
            it->second = value;
            return;
        }

        map.emplace(std::string{key}, value);
    }

    template <typename Map, typename Generation = typename Map::mapped_type>
    static Generation lookup(const Map& map, std::string_view key)
    {
        auto it = map.find(key);
        return it != map.end() ? it->second : Generation{0};
    }

private:
    struct string_view_less
    {
        using is_transparent = void;

        bool operator()(std::string_view lhs, std::string_view rhs) const noexcept
        {
            return lhs < rhs;
        }
    };

    using generation_map = std::map<std::string, generation, string_view_less>;

    generation current_{1};
    generation_map node_generation_;
    generation_map subtree_generation_;
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

struct duplicate_child_key_error : resource_error
{
    const char* what() const noexcept override
    {
        return "duplicate child key";
    }
};

} // namespace kl::resources
