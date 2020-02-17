#pragma once

#include <type_traits>

namespace kl {

template <typename Enum>
class enum_flags
{
    static_assert(std::is_enum<Enum>::value, "Enum is not a enum-type");

public:
    using underlying_type = std::underlying_type_t<Enum>;
    using enum_type = Enum;

public:
    constexpr enum_flags() noexcept : value_{0} {}
    constexpr explicit enum_flags(Enum value) noexcept
        : value_{static_cast<underlying_type>(value)}
    {
    }

    constexpr underlying_type underlying_value() const noexcept
    {
        return value_;
    }
    constexpr Enum value() const noexcept { return static_cast<Enum>(value_); }

    constexpr explicit operator bool() const noexcept { return value_ != 0; }

    constexpr bool test(Enum flag) const noexcept
    {
        return (value_ & static_cast<underlying_type>(flag)) ==
               static_cast<underlying_type>(flag);
    }

    constexpr enum_flags operator~() const noexcept
    {
        return enum_flags{static_cast<enum_type>(~value_)};
    }

    constexpr enum_flags operator&(enum_flags f) const noexcept
    {
        return enum_flags{static_cast<enum_type>(value_ & f.value_)};
    }

    constexpr enum_flags operator|(enum_flags f) const noexcept
    {
        return enum_flags{static_cast<enum_type>(value_ | f.value_)};
    }

    constexpr enum_flags operator^(enum_flags f) const noexcept
    {
        return enum_flags{static_cast<enum_type>(value_ ^ f.value_)};
    }

    constexpr enum_flags& operator&=(enum_flags f) noexcept
    {
        value_ &= f.value_;
        return *this;
    }

    constexpr enum_flags& operator|=(enum_flags f) noexcept
    {
        value_ |= f.value_;
        return *this;
    }

    constexpr enum_flags& operator^=(enum_flags f) noexcept
    {
        value_ ^= f.value_;
        return *this;
    }

    constexpr enum_flags operator&(Enum value) const noexcept
    {
        return (*this & enum_flags{value});
    }

    constexpr enum_flags operator|(Enum value) const noexcept
    {
        return (*this | enum_flags{value});
    }

    constexpr enum_flags operator^(Enum value) const noexcept
    {
        return (*this ^ enum_flags{value});
    }

    constexpr enum_flags& operator&=(Enum value) noexcept
    {
        return (*this &= enum_flags{value});
    }

    constexpr enum_flags& operator|=(Enum value) noexcept
    {
        return (*this |= enum_flags{value});
    }

    constexpr enum_flags& operator^=(Enum value) noexcept
    {
        return (*this ^= enum_flags{value});
    }

    constexpr friend enum_flags operator|(Enum a, enum_flags b) noexcept
    {
        return enum_flags{a} | b;
    }

    constexpr friend enum_flags operator&(Enum a, enum_flags b) noexcept
    {
        return enum_flags{a} & b;
    }

    constexpr friend enum_flags operator^(Enum a, enum_flags b) noexcept
    {
        return enum_flags{a} ^ b;
    }

private:
    constexpr explicit enum_flags(underlying_type value) noexcept
        : value_{value}
    {
    }

private:
    underlying_type value_;
};

template <typename Enum>
constexpr enum_flags<Enum> make_flags(Enum value) noexcept
{
    return enum_flags<Enum>{value};
}

template <typename T>
struct is_enum_flags : std::false_type {};

template <typename T>
struct is_enum_flags<enum_flags<T>> : std::true_type {};

} // namespace kl
