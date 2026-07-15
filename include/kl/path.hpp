#pragma once

#include <boost/container/small_vector.hpp>

#include <cassert>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

namespace kl {

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

    std::string_view prefix(std::size_t count) const noexcept
    {
        assert(count <= size_);
        if (count == 0)
            return {};

        const auto segment = segments_[count - 1];
        return std::string_view{*value_}.substr(0, segment.offset + segment.length);
    }

private:
    path_view(const std::string& value, const path_segment* segments, std::size_t size) noexcept
        : value_{&value}, segments_{segments}, size_{size}
    {
    }

    const std::string* value_ = nullptr;
    const path_segment* segments_ = nullptr;
    std::size_t size_ = 0;

    friend class path;
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
    explicit path(std::string_view value)
    {
        set(value);
    }

    void set(std::string_view value);

    void clear() noexcept;

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

    std::string_view joined(std::size_t offset) const noexcept
    {
        if (offset >= segments_.size())
            return {};
        return std::string_view{value_}.substr(segments_[offset].offset);
    }

private:
    [[nodiscard]] static int from_xdigit(char ch) noexcept;

    void pop_segment();
    void push_decoded_segment(std::string_view segment);

    std::string value_;
    segment_container segments_;
};

} // namespace kl
