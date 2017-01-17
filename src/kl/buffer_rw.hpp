#pragma once

#include "kl/byte.hpp"
#include "kl/type_traits.hpp"

#include <gsl/span>
#include <cstdint>
#include <cstring>
#include <vector>

namespace kl {

class buffer_reader
{
public:
    explicit buffer_reader(gsl::span<const byte> buffer) noexcept
        : buffer_(std::move(buffer))
    {
    }

    template <typename T, std::ptrdiff_t Extent,
              typename = enable_if<std::is_trivial<T>>>
    explicit buffer_reader(gsl::span<T, Extent> buffer) noexcept
        : buffer_reader({reinterpret_cast<const byte*>(buffer.data()),
                         buffer.size_bytes()})
    {
    }

    template <typename T>
    bool read(T& value)
    {
        if (err_)
            return false;
        (*this) >> value;
        return !err_;
    }

    template <typename T,
              typename = enable_if<std::is_default_constructible<T>>>
    T read()
    {
        T value{};
        read(value);
        return value;
    }

    template <typename T>
    bool peek(T& value) noexcept
    {
        return peek_basic(value);
    }

    template <typename T,
              typename = enable_if<std::is_default_constructible<T>>>
    T peek()
    {
        T value{};
        peek(value);
        return value;
    }

    gsl::span<const byte> view(std::size_t count,
                               bool move_cursor = true) noexcept
    {
        // If left() is negative we already have err_ set to true
        if (count > static_cast<std::size_t>(left()))
            err_ = true;
        if (err_)
            return {};

        gsl::span<const byte> ret(cursor(), count);
        if (move_cursor)
            pos_ += count;

        return ret;
    }

    void skip(std::ptrdiff_t off) noexcept
    {
        // Make sure pos_ does not escape of buffer range
        if (off > 0 && off > left())
            err_ = true;
        if (off < 0 && (-off) > pos_)
            err_ = true;

        if (!err_)
            pos_ += off;
    }

    bool empty() const noexcept { return left() <= 0; }
    // Returns how many bytes are left in the internal buffer
    std::ptrdiff_t left() const noexcept { return buffer_.size_bytes() - pos(); }
    // Returns how many bytes we've already read
    std::ptrdiff_t pos() const noexcept { return pos_; }

    bool err() const noexcept { return err_; }

    // Useful in user-provided operator<< for composite types to fail fast
    void notify_error() noexcept { err_ = true; }

    // Default Stream Op implementation for all trivially copyable types
    template <typename T>
    friend buffer_reader& operator>>(buffer_reader& r, T& value)
    {
        // If you get compilation error here it means your type T does not
        // provide operator>>(kl::buffer_reader&, T&) function and does not
        // satisfy TriviallyCopyable concept

        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        if (!r.err_ && r.peek_basic(value))
        {
            r.pos_ += sizeof(value);
        }
        else
        {
            r.err_ = true;
        }

        return r;
    }

    template <typename T, std::ptrdiff_t Extent>
    friend buffer_reader& operator>>(buffer_reader& r,
                                     gsl::span<T, Extent> span)
    {
        if (!r.err_ && r.peek_span(span))
        {
            r.pos_ += span.size_bytes();
        }
        else
        {
            r.err_ = true;
        }

        return r;
    }

private:
    const byte* cursor() const noexcept
    {
        return buffer_.data() + pos_;
    }

    template <typename T>
    bool peek_basic(T& value) noexcept
    {
        // If the objects are not TriviallyCopyable, the behavior of memcpy is
        // not specified and may be undefined
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        // If left() is negative we already have err_ set to true
        if (err_ || static_cast<std::size_t>(left()) < sizeof(T))
            return false;

        std::memcpy(&value, cursor(), sizeof(T));
        return true;
    }

    template <typename T, std::ptrdiff_t Extent>
    bool peek_span(gsl::span<T, Extent> span)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        auto repr = gsl::make_span(reinterpret_cast<byte*>(span.data()),
                                   span.size_bytes());

        if (err_ || left() < repr.size_bytes())
            return false;

        std::memcpy(repr.data(), cursor(), repr.size_bytes());
        return true;
    }

private:
    gsl::span<const byte> buffer_;
    std::ptrdiff_t pos_{0};
    bool err_{false};
};
} // namespace kl
