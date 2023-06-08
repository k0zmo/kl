#pragma once

#include "kl/utility.hpp"

#include <cstddef>
#include <type_traits>

namespace kl {

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
    using signature_type = Ret(Args...);
    using args_type = type_pack<Args...>;
    static const std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct arg
    {
        using type = at_type_t<N, Args...>;
    };

    template <std::size_t N>
    using arg_t = typename arg<N>::type;
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
struct func_traits<F&> : func_traits<F> {};

template <typename F>
struct func_traits<F&&> : func_traits<F> {};

template <typename T>
struct always_false : std::false_type {};

template <typename T>
inline constexpr bool always_false_v = always_false<T>::value;

template <typename T, typename U, typename... Rest>
struct is_same;

template <typename... Ts>
inline constexpr bool is_same_v = is_same<Ts...>::value;

// Variadic is_same<T...>
template <typename T, typename U, typename... Rest>
struct is_same : std::integral_constant<bool, std::is_same_v<T, U> &&
                                                  is_same_v<U, Rest...>>
{
};

template <typename T, typename U>
struct is_same<T, U> : std::integral_constant<bool, std::is_same_v<T, U>>
{
};
} // namespace kl

#define KL_HAS_TYPEDEF_HELPER(type)                                            \
    template <typename T, typename = void>                                     \
    struct has_##type : std::false_type {};                                    \
    template <typename T>                                                      \
    struct has_##type<T, std::void_t<typename T::type>> : std::true_type {};   \
    template <typename T>                                                      \
    inline constexpr bool has_##type##_v = has_##type<T>::value;

#define KL_VALID_EXPR_HELPER(name, expr)                                       \
    template <typename T, typename = void>                                     \
    struct name : std::false_type {};                                          \
    template <typename T>                                                      \
    struct name<T, std::void_t<decltype(expr)>> : std::true_type {};           \
    template <typename T>                                                      \
    inline constexpr bool name##_v = name<T>::value;
