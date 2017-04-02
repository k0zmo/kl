#pragma once

#include <type_traits>
#include <utility>
#include <array>
#include <cstring>

namespace kl {

// Casts any kind of enum (enum, enum class) to its underlying type (i.e int32_t)
template <typename Enum>
constexpr auto underlying_cast(Enum e)
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#if defined(_MSC_VER) && _MSC_VER < 1900
template <typename T>
struct array_size;

template <typename T, std::size_t N>
struct array_size<T[N]> : std::integral_constant<std::size_t, N>
{
};

template <typename T, std::size_t N>
struct array_size<std::array<T, N>> : std::integral_constant<std::size_t, N>
{
};
#define countof(x) (kl::array_size<decltype(x)>::value)
#else
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
#endif

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

template <typename T>
struct identity_type
{
    using type = T;
};

template <typename T>
using identity_type_t = typename identity_type<T>::type;

/*
 * Query traits of function (can be function, lambda, functor or member
 * function)
 * - typename func_traits<Func>::return_type => return type of Func
 * - func_traits<Func>::arity => number of function arguments
 * - typename func_traits<Func>::class_type => class type of Func for member
 * function
 * - typename func_traits<Func>::template arg<1>::type => type of second
 * argument of Func
 */
template <typename F>
struct func_traits;

template <typename Ret, typename... Args>
struct func_traits<Ret(Args...)>
{
    using return_type = Ret;
    // MSVC2013 doesn't like: using signature_type = Ret(Args...);
    using signature_type = identity_type_t<Ret(Args...)>;
    using args_type = type_pack<Args...>;
    static const std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct arg
    {
        using type = at_type_t<N, Args...>;
    };
};

// Lambdas and anything else having operator() defined
template <typename F>
struct func_traits
    : func_traits<decltype(&std::remove_reference_t<F>::operator())>
{
};

// Function pointers
template <typename Ret, typename... Args>
struct func_traits<Ret (*)(Args...)> : func_traits<Ret(Args...)>
{
};

// Member function pointers
template <typename Class, typename Ret, typename... Args>
struct func_traits<Ret (Class::*)(Args...)> : func_traits<Ret(Args...)>
{
    using class_type = Class;
};

// Member const-function pointers
template <typename Class, typename Ret, typename... Args>
struct func_traits<Ret (Class::*)(Args...) const> : func_traits<Ret(Args...)>
{
    using class_type = Class;
};

template <typename F>
struct func_traits<F&> : func_traits<F>
{
};

template <typename F>
struct func_traits<F&&> : func_traits<F>
{
};

template <typename T>
struct always_false : std::false_type
{
};

template <typename T, typename U, typename... Rest>
struct is_same : std::integral_constant<bool, std::is_same<T, U>::value &&
                                                  is_same<U, Rest...>::value>
{
};

template <typename T, typename U>
struct is_same<T, U> : std::integral_constant<bool, std::is_same<T, U>::value>
{
};

struct has_call_operator_test
{
    // SFINAE for expressions: VS2013 seems to be OK with simple expressions
    // if U::operator() expression is malformed return type is ill-formed
    // and whole function is removed from ADL
    template <typename U>
    static auto test(U*) -> decltype(&U::operator(), std::true_type());

    template <typename U>
    static auto test(...) -> std::false_type;
};

// Checks if given type T has a unambiguous operator()
template <typename T>
struct has_call_operator
    : decltype(has_call_operator_test::template test<T>(nullptr))
{
};

// C++17 stuff
template <bool value>
using bool_constant = std::integral_constant<bool, value>;

template <typename...>
struct conjunction : std::true_type {};

template <typename B1>
struct conjunction<B1> : B1 {};

template <typename B1, typename... Bn>
struct conjunction<B1, Bn...>
    : std::conditional_t<B1::value != false, conjunction<Bn...>, B1>
{
};

template <typename...>
struct disjunction : std::false_type {};

template <typename B1>
struct disjunction<B1> : B1 {};

template <typename B1, typename... Bn>
struct disjunction<B1, Bn...>
    : std::conditional_t<B1::value != false, B1, disjunction<Bn...>>
{
};

template <typename B>
struct negation : bool_constant<!B::value> {};

// Used as template parameter like so: enable_if<std::is_integral<T>> = 0
template <typename... Ts>
using enable_if = std::enable_if_t<conjunction<Ts...>::value, int>;

// C++14 stuff for MSVC2013
template <typename...>
struct make_void_t { using type = void; };
template <typename... Ts>
using void_t = typename make_void_t<Ts...>::type;
} // namespace kl
