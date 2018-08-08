#pragma once

#include <array>
#include <type_traits>
#include <cstring>

namespace kl {

// Casts any kind of enum (enum, enum class) to its underlying type (i.e
// int32_t)
template <typename Enum>
constexpr auto underlying_cast(Enum e)
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename T, std::size_t N>
constexpr auto countof(const T (&arr)[N])
{
    return N;
}

template <typename T, std::size_t N>
constexpr auto countof(const std::array<T, N>& arr)
{
    return N;
}

template <typename To, typename From>
To bitwise_cast(From from)
{
    static_assert(sizeof(To) == sizeof(From),
                  "Size of destination and source objects must be equal.");
    static_assert(std::is_trivially_copyable<To>::value,
                  "To type must be trivially copyable.");
    static_assert(std::is_trivially_copyable<From>::value,
                  "From type must be trivially copyable");

    To to;
    std::memcpy(&to, &from, sizeof(to));
    return to;
}
} // namespace kl
