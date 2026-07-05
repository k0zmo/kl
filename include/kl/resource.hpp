#pragma once

#include "kl/utility.hpp"

#include <boost/container/small_vector.hpp>

#include <exception>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace kl::resources {

struct path_segment
{
    std::size_t offset;
    std::size_t length;
};

namespace detail {

inline std::string_view path_segment_at(const std::string& value,
                                        const path_segment* segments,
                                        std::size_t i) noexcept
{
    return std::string_view{value}.substr(segments[i].offset, segments[i].length);
}

} // namespace detail

class path_view
{
public:
    path_view() = default;

    bool empty() const noexcept { return size() == 0; }
    std::size_t size() const noexcept { return size_; }

    std::string_view operator[](std::size_t i) const noexcept
    {
        return detail::path_segment_at(*value_, segments_, i);
    }

    std::string_view front() const noexcept { return (*this)[0]; }
    path_view drop_front() const noexcept
    {
        return path_view{*value_, segments_ + 1, size_ - 1};
    }

    path_view drop_back() const noexcept
    {
        return path_view{*value_, segments_, size_ - 1};
    }

private:
    // Used by modification_tracker
    std::string_view prefix_key(std::size_t count) const noexcept
    {
        if (count == 0)
            return {};

        const auto segment = segments_[count - 1];
        return std::string_view{*value_}.substr(0, segment.offset + segment.length);
    }

    // Used by path
    path_view(const std::string& value, const path_segment* segments, std::size_t size) noexcept
        : value_{&value}, segments_{segments}, size_{size}
    {
    }

private:
    const std::string* value_ = nullptr;
    const path_segment* segments_ = nullptr;
    std::size_t size_ = 0;

    friend class path;
    friend class modification_tracker;
};

class path_parse_error : public std::exception
{
public:
    explicit path_parse_error(const char* message) noexcept
        : message_{message}
    {
    }

    const char* what() const noexcept override
    {
        return message_;
    }

private:
    const char* message_;
};

class path
{
private:
    using segment_container = boost::container::small_vector<path_segment, 15>;

public:
    path() = default;
    explicit path(std::string_view value) { set(std::move(value)); }

    void set(std::string_view value)
    {
        clear();

        if (value.empty() || value == "/")
            return;

        value_.reserve(value.size());
        if (value.front() == '/')
            value.remove_prefix(1);

        while (true)
        {
            const auto pos = value.find('/');
            const auto length = pos == std::string_view::npos ? value.size() : pos;
            const auto segment = value.substr(0, length);

            if (!segment.empty())
            {
                if (segment == ".")
                {
                    // Skip
                }
                else if (segment == "..")
                {
                    pop_segment();
                }
                else
                {
                    push_decoded_segment(segment);
                }
            }

            if (pos == std::string_view::npos)
                break;

            value.remove_prefix(pos + 1);
        }
    }

    void clear() noexcept
    {
        value_.clear();
        segments_.clear();
    }

    bool empty() const noexcept { return segments_.empty(); }
    std::size_t size() const noexcept { return segments_.size(); }
    path_view view() const noexcept
    {
        return path_view{value_, segments_.data(), segments_.size()};
    }

    std::string_view operator[](std::size_t i) const noexcept
    {
        return detail::path_segment_at(value_, segments_.data(), i);
    }

private:
    constexpr static int from_xdigit(char ch) noexcept
    {
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        if (ch >= 'A' && ch <= 'F')
            return 10 + (ch - 'A');
        if (ch >= 'a' && ch <= 'f')
            return 10 + (ch - 'a');
        return -1;
    }

    void pop_segment()
    {
        if (segments_.empty())
            return;

        const auto offset = segments_.back().offset;
        value_.erase(offset == 0 ? offset : offset - 1);
        segments_.pop_back();
    }

    void push_decoded_segment(std::string_view segment)
    {
        if (!value_.empty())
            value_ += '/';

        const auto offset = value_.size();

        for (std::size_t i = 0; i < segment.size();)
        {
            if (segment[i] != '%')
            {
                value_ += segment[i++];
                continue;
            }

            if (i + 2 >= segment.size())
                throw path_parse_error{"invalid percent-encoded path segment"};

            const auto hi = from_xdigit(segment[i + 1]);
            const auto lo = from_xdigit(segment[i + 2]);
            if (hi < 0 || lo < 0)
                throw path_parse_error{"invalid percent-encoded path segment"};

            const auto ch = static_cast<char>((hi << 4) | lo);
            if (ch == '/')
                throw path_parse_error{"path segment cannot contain '/'"};

            value_ += ch;
            i += 3;
        }

        segments_.push_back({offset, value_.size() - offset});
    }

private:
    std::string value_;
    segment_container segments_;
};

class modification_tracker
{
public:
    enum class generation : std::uint64_t {};

    generation current() const noexcept { return current_;}

    generation notify_changed(const kl::resources::path& path, bool subtree = false)
    {
        return notify_changed(path.view(), subtree);
    }

    generation notify_changed(path_view path, bool subtree = false)
    {
        auto gen = generation{underlying_cast(current_) + 1};
        current_ = gen;

        // Mark the node and all ancestors, so parent watchers observe child changes
        for (std::size_t i = 0; i <= path.size(); ++i)
            assign(node_generation_, path.prefix_key(i), gen);

        // If a whole subtree was replaced, descendants should also appear changed,
        // without walking every reflected child.
        if (subtree)
            assign(subtree_generation_, path.prefix_key(path.size()), gen);

        return gen;
    }

    generation changed_at(const kl::resources::path& path) const
    {
        return changed_at(path.view());
    }

    generation changed_at(path_view path) const
    {
        auto result = generation{0};

        // If any ancestor subtree was replaced, this path changed too.
        for (std::size_t i = 0U; i <= path.size(); ++i)
        {
            const auto key = path.prefix_key(i);

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

    bool changed_since(const kl::resources::path& path, generation old) const
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
