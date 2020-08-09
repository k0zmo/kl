#pragma once

#include "kl/type_traits.hpp"

#include <gsl/span>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kl {

namespace detail {

template <typename T>
class cursor_base
{
    // Expect T to differ only by CV-qualifier
    static_assert(std::is_same<std::remove_cv_t<T>, std::byte>::value, "!!!");

public:
    explicit cursor_base(gsl::span<T> buffer) noexcept
        : buffer_(std::move(buffer))
    {
    }

    template <typename U, std::size_t Extent>
    explicit cursor_base(gsl::span<U, Extent> buffer) noexcept
        : buffer_{reinterpret_cast<T*>(buffer.data()), buffer.size_bytes()}
    {
    }

    void skip(std::ptrdiff_t off) noexcept
    {
        // Make sure pos_ does not escape of buffer range
        if (off > 0 && static_cast<std::size_t>(off) > left())
            err_ = true;
        if (off < 0 && static_cast<std::size_t>(-off) > pos_)
            err_ = true;

        if (!err_)
            pos_ += off;
    }

    bool empty() const noexcept { return left() <= 0; }
    // Returns how many bytes are left in the internal buffer
    std::size_t left() const noexcept
    {
        return buffer_.size_bytes() - pos();
    }
    // Returns how many bytes we've already read
    std::size_t pos() const noexcept { return pos_; }

    bool err() const noexcept { return err_; }

    // Useful in user-provided operator>> for composite types to fail fast
    void notify_error() noexcept { err_ = true; }

protected:
    T* cursor() const noexcept { return buffer_.data() + pos_; }

protected:
    gsl::span<T> buffer_;
    std::size_t pos_{0};
    bool err_{false};
};

template <typename T>
using is_simple =
    std::bool_constant<std::is_arithmetic<T>::value || std::is_enum<T>::value>;

} // namespace detail

class binary_reader : public detail::cursor_base<const std::byte>
{
public:
    using detail::cursor_base<const std::byte>::cursor_base;

    template <typename T>
    bool peek(T& value) noexcept
    {
        // If the objects are not TriviallyCopyable, the behavior of memcpy is
        // not specified and may be undefined.
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        return peek_impl(reinterpret_cast<std::byte*>(&value), sizeof(T));
    }

    template <typename T>
    bool read_raw(T& value) noexcept
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        return read_impl(reinterpret_cast<std::byte*>(&value), sizeof(T));
    }

    template <typename T, std::size_t Extent>
    bool read_span(gsl::span<T, Extent> span) noexcept
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        return read_impl(reinterpret_cast<std::byte*>(span.data()),
                         span.size_bytes());
    }

public:
    template <typename T>
    bool read(T& value)
    {
        if (err_)
            return false;
        read_binary(*this, value);
        return !err_;
    }

    template <typename T,
              enable_if<std::is_default_constructible<T>> = true>
    T read()
    {
        T value{};
        read(value);
        return value;
    }

    template <typename T,
              enable_if<std::is_default_constructible<T>> = true>
    T peek()
    {
        T value{};
        peek(value);
        return value;
    }

    // Does not copy the data
    gsl::span<const std::byte> span(std::size_t count, bool move_cursor = true)
    {
        if (count > static_cast<std::size_t>(left()))
            err_ = true;
        if (err_)
            return {};

        gsl::span<const std::byte> ret(cursor(), count);
        if (move_cursor)
            pos_ += count;

        return ret;
    }

private:
    bool peek_impl(std::byte* data, std::size_t size) noexcept
    {
        if (err_ || static_cast<std::size_t>(left()) < size)
            return false;

        std::memcpy(data, cursor(), size);
        return true;
    }

    bool read_impl(std::byte* data, std::size_t size) noexcept
    {
        if (peek_impl(data, size))
            pos_ += size;
        else
            err_ = true;

        return !err_;
    }
};

// read_binary implementation for all basic types + enum types
template <typename T, enable_if<detail::is_simple<T>> = true>
void read_binary(binary_reader& r, T& value) noexcept
{
    r.read_raw(value);
}

// read_binary implementation for all basic types + enum types
template <typename T, std::size_t Extent>
void read_binary(binary_reader& r, gsl::span<T, Extent> span)
{
    r.read_span(span);
}

template <typename T>
binary_reader& operator>>(binary_reader& r, T& value)
{
    read_binary(r, value);
    return r;
}

// This must be present to allow for rvalue spans
template <typename T, std::size_t Extent>
binary_reader& operator>>(binary_reader& r, gsl::span<T, Extent> span)
{
    read_binary(r, span);
    return r;
}

class binary_writer : public detail::cursor_base<std::byte>
{
public:
    using detail::cursor_base<std::byte>::cursor_base;

    template <typename T>
    bool write_raw(const T& value) noexcept
    {
        // If the objects are not TriviallyCopyable, the behavior of memcpy is
        // not specified and may be undefined.
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        return write_impl(reinterpret_cast<const std::byte*>(&value),
                          sizeof(T));
    }

    template <typename T, std::size_t Extent>
    bool write_span(gsl::span<const T, Extent> span) noexcept
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be a trivially copyable type");

        return write_impl(reinterpret_cast<const std::byte*>(span.data()),
                          span.size_bytes());
    }

private:
    bool write_impl(const std::byte* data, std::size_t size) noexcept
    {
        if (err_ || static_cast<std::size_t>(left()) < size)
        {
            err_ = true;
            return false;
        }

        std::memcpy(cursor(), data, size);
        pos_ += size;

        return true;
    }
};

// write_binary implementation for all basic types + enum types
template <typename T, enable_if<detail::is_simple<T>> = true>
void write_binary(binary_writer& w, const T& value) noexcept
{
    w.write_raw(value);
}

// write_binary implementation for spans
template <typename T, std::size_t Extent>
void write_binary(binary_writer& w, gsl::span<const T, Extent> span) noexcept
{
    w.write_span(span);
}

template <typename T>
binary_writer& operator<<(binary_writer& w, const T& value)
{
    write_binary(w, value);
    return w;
}
} // namespace kl
