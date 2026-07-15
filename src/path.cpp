#include <cstddef>
#include <kl/path.hpp>

#include <string_view>

namespace kl {

void path::set(std::string_view value)
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

void path::clear() noexcept
{
    value_.clear();
    segments_.clear();
}

int path::from_xdigit(char ch) noexcept
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return 10 + (ch - 'A');
    if (ch >= 'a' && ch <= 'f')
        return 10 + (ch - 'a');
    return -1;
}

void path::pop_segment()
{
    if (segments_.empty())
        return;

    const auto offset = segments_.back().offset;
    value_.erase(offset == 0 ? offset : offset - 1);
    segments_.pop_back();
}

void path::push_decoded_segment(std::string_view segment)
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

} // namespace kl
