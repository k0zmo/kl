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
template <typename OutStream, typename DelimT>
class outstream_joiner
{
public:
    using outstream_type = OutStream;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    outstream_joiner(OutStream& stream, const DelimT& delimiter)
        : stream_{std::addressof(stream)}, delimiter_{delimiter}
    {
    }

    outstream_joiner(OutStream& stream, DelimT&& delimiter)
        : stream_{std::addressof(stream)}, delimiter_{std::move(delimiter)}
    {
    }

    outstream_joiner(const outstream_joiner& other) = default;
    outstream_joiner(outstream_joiner&& other) = default;

    outstream_joiner& operator=(const outstream_joiner& other) = default;
    outstream_joiner& operator=(outstream_joiner&& other) = default;

    // No-op
    outstream_joiner& operator*() { return *this; }
    outstream_joiner& operator++() { return *this; }
    outstream_joiner& operator++(int) { return *this; }

    template <typename T>
    outstream_joiner& operator=(const T& value)
    {
        if (!first_)
            *stream_ << delimiter_;
        first_ = false;
        *stream_ << value;
        return *this;
    }

private:
    OutStream* stream_;
    DelimT delimiter_;
    bool first_{true};
};

template <typename OutStream, typename DelimT>
outstream_joiner<OutStream, std::decay_t<DelimT>>
    make_outstream_joiner(OutStream& os, DelimT&& delimiter)
{
    return outstream_joiner<OutStream, std::decay_t<DelimT>>(
        os, std::forward<DelimT>(delimiter));
}
} // namespace kl
