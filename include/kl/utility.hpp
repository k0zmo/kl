#pragma once

#include <type_traits>
#include <cstring>

namespace kl {

// Casts any kind of enum (enum, enum class) to its underlying type (i.e
// int32_t)
template <typename Enum>
constexpr auto underlying_cast(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

// Use it as template parameter like so: enable_if<std::is_integral<T>> = true
template <typename... Ts>
using enable_if = std::enable_if_t<std::conjunction_v<Ts...>, bool>;

template <typename To, typename From,
          enable_if<std::bool_constant<sizeof(To) == sizeof(From)>,
                    std::is_trivially_copyable<To>,
                    std::is_trivially_copyable<From>> = true>
To bit_cast(const From& from) noexcept
{
    static_assert(std::is_default_constructible_v<To>,
                  "To must be default constructible");
    To to;
    std::memcpy(&to, &from, sizeof(to));
    return to;
}

// From https://vector-of-bool.github.io/2017/08/12/partial-specializations.html
template <typename T>
struct type_t {};

template <typename T>
constexpr type_t<T> type{};

/*
 * Get N-th type from the list of types (0-based)
 * Usage: at_type_t<1, int, double&> => double&
 */
template <unsigned N, typename Head, typename... Tail>
struct at_type
{
    static_assert(N < 1 + sizeof...(Tail), "invalid arg index");
    using type = typename at_type<N - 1, Tail...>::type;
};

template <typename Head, typename... Tail>
struct at_type<0, Head, Tail...>
{
    using type = Head;
};

template <unsigned N, typename... Args>
using at_type_t = typename at_type<N, Args...>::type;

template <typename... Ts>
struct type_pack : std::integral_constant<std::size_t, sizeof...(Ts)>
{
    template <std::size_t N>
    struct extract
    {
        using type = at_type_t<N, Ts...>;
    };
};

// Instantiate printer<T> deep in template territory to get compile error
// message with a T name
template <typename T>
struct printer;
} // namespace kl
