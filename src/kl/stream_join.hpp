#pragma once

#include <iosfwd>
#include <iterator>

namespace kl {

template <typename Iterable>
struct stream_joiner
{
    stream_joiner(const Iterable& xs) : xs_{xs} {}

    stream_joiner& set_delimiter(const char* delim)
    {
        delim_ = delim;
        return *this;
    }

    stream_joiner& set_empty_string(const char* empty)
    {
        empty_string_ = empty;
        return *this;
    }

    template <typename OutStream>
    friend OutStream& operator<<(OutStream& os, const stream_joiner& sj)
    {
        using std::begin;
        using std::end;

        auto i = begin(sj.xs_);
        auto e = end(sj.xs_);
        if (i != e)
        {
            for (os << *i++; i != e; ++i)
                os << sj.delim_ << *i;
        }
        else
        {
            os << sj.empty_string_;
        }

        return os;
    }

private:
    const Iterable& xs_;
    const char* delim_{", "};
    const char* empty_string_{"."};
};

// Print iterable (begin, end) as x0, x1, x2, ..., xn
template <typename Iterable>
stream_joiner<Iterable> stream_join(const Iterable& xs)
{
    return stream_joiner<Iterable>{xs};
}

// Based on http://en.cppreference.com/w/cpp/experimental/ostream_joiner
template <typename DelimT, typename CharT = char,
          typename Traits = std::char_traits<CharT>>
class ostream_joiner
{
public:
    using char_type = CharT;
    using traits_type = Traits;
    using ostream_type = std::basic_ostream<CharT, Traits>;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    ostream_joiner(ostream_type& stream, const DelimT& delimiter)
        : stream_{std::addressof(stream)}, delimiter_{delimiter}
    {
    }

    ostream_joiner(ostream_type& stream, DelimT&& delimiter)
        : stream_{std::addressof(stream)}, delimiter_{std::move(delimiter)}
    {
    }

    ostream_joiner(const ostream_joiner& other) = default;
    ostream_joiner(ostream_joiner&& other) = default;

    ostream_joiner& operator=(const ostream_joiner& other) = default;
    ostream_joiner& operator=(ostream_joiner&& other) = default;

    // No-op
    ostream_joiner& operator*() { return *this; }
    ostream_joiner& operator++() { return *this; }
    ostream_joiner& operator++(int) { return *this; }

    template <typename T>
    ostream_joiner& operator=(const T& value)
    {
        if (!first_)
            *stream_ << delimiter_;
        first_ = false;
        *stream_ << value;
        return *this;
    }

private:
    ostream_type* stream_;
    DelimT delimiter_;
    bool first_{true};
};

template <typename CharT, typename Traits, typename DelimT>
ostream_joiner<std::decay_t<DelimT>, CharT, Traits>
    make_ostream_joiner(std::basic_ostream<CharT, Traits>& os,
                        DelimT&& delimiter)
{
    return ostream_joiner<std::decay_t<DelimT>, CharT, Traits>(
        os, std::forward<DelimT>(delimiter));
}
} // namespace kl
