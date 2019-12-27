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
    constexpr enum_flags() : value_{0} {}
    constexpr explicit enum_flags(Enum value)
        : value_{static_cast<underlying_type>(value)}
    {
    }

    constexpr underlying_type underlying_value() const { return value_; }
    constexpr Enum value() const { return static_cast<Enum>(value_); }

    constexpr explicit operator bool() const { return value_ != 0; }

    constexpr bool test(Enum flag) const
    {
        return (value_ & static_cast<underlying_type>(flag)) ==
               static_cast<underlying_type>(flag);
    }

    constexpr enum_flags operator~() const { return enum_flags(~value_); }

    constexpr enum_flags operator&(enum_flags f) const
    {
        return enum_flags{value_ & f.value_};
    }

    constexpr enum_flags operator|(enum_flags f) const
    {
        return enum_flags{value_ | f.value_};
    }

    constexpr enum_flags operator^(enum_flags f) const
    {
        return enum_flags{value_ ^ f.value_};
    }

    constexpr enum_flags& operator&=(enum_flags f)
    {
        value_ &= f.value_;
        return *this;
    }

    constexpr enum_flags& operator|=(enum_flags f)
    {
        value_ |= f.value_;
        return *this;
    }

    constexpr enum_flags& operator^=(enum_flags f)
    {
        value_ ^= f.value_;
        return *this;
    }

    constexpr enum_flags operator&(Enum value) const
    {
        return (*this & enum_flags{value});
    }

    constexpr enum_flags operator|(Enum value) const
    {
        return (*this | enum_flags{value});
    }

    constexpr enum_flags operator^(Enum value) const
    {
        return (*this ^ enum_flags{value});
    }

    constexpr enum_flags& operator&=(Enum value)
    {
        return (*this &= enum_flags{value});
    }

    constexpr enum_flags& operator|=(Enum value)
    {
        return (*this |= enum_flags{value});
    }

    constexpr enum_flags& operator^=(Enum value)
    {
        return (*this ^= enum_flags{value});
    }

    constexpr friend enum_flags operator|(Enum a, enum_flags b)
    {
        return enum_flags{a} | b;
    }

    constexpr friend enum_flags operator&(Enum a, enum_flags b)
    {
        return enum_flags{a} & b;
    }

    constexpr friend enum_flags operator^(Enum a, enum_flags b)
    {
        return enum_flags{a} ^ b;
    }

private:
    constexpr explicit enum_flags(underlying_type value) : value_{value} {}

private:
    underlying_type value_;
};

template <typename Enum>
constexpr enum_flags<Enum> make_flags(Enum value)
{
    return enum_flags<Enum>{value};
}

template <typename T>
struct is_enum_flags : std::false_type {};

template <typename T>
struct is_enum_flags<enum_flags<T>> : std::true_type {};

} // namespace kl
