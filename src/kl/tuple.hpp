#pragma once

#include <tuple>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <initializer_list>

namespace kl {

using std::index_sequence;
using std::make_index_sequence;

template <typename T>
using make_tuple_indices =
    make_index_sequence<std::tuple_size<std::remove_reference_t<T>>::value>;

namespace tuple {

// Applies a function to given tuple
struct apply_fn
{
private:
    template <typename Tup, typename Fun, std::size_t... Is>
    static auto impl(Tup&& tup, Fun&& fun, index_sequence<Is...>) -> decltype(
        std::forward<Fun>(fun)((std::get<Is>(std::forward<Tup>(tup)))...))
    {
        return std::forward<Fun>(fun)(
            (std::get<Is>(std::forward<Tup>(tup)))...);
    }

public:
    template <typename Tup, typename Fun>
    static auto call(Tup&& tup, Fun&& fun)
        -> decltype(apply_fn::impl(std::forward<Tup>(tup),
                                   std::forward<Fun>(fun),
                                   make_tuple_indices<Tup>{}))
    {
        return apply_fn::impl(std::forward<Tup>(tup), std::forward<Fun>(fun),
                              make_tuple_indices<Tup>{});
    }
};

// Transforms tuple of dereferencable to tuple of references:
//   tuple<int*, boost::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int&, double&, const short&>
struct transform_ref_fn
{
private:
    template <typename Tup, std::size_t... Is>
    static auto impl(Tup&& tup, index_sequence<Is...>) -> decltype(
        std::tuple<decltype(*(std::get<Is>(std::forward<Tup>(tup))))...>{
            (*std::get<Is>(std::forward<Tup>(tup)))...})
    {
        return std::tuple<decltype(*(std::get<Is>(std::forward<Tup>(tup))))...>{
            (*std::get<Is>(std::forward<Tup>(tup)))...};
    }

public:
    template <typename Tup>
    static auto call(Tup&& tup)
        -> decltype(transform_ref_fn::impl(std::forward<Tup>(tup),
                                           make_tuple_indices<Tup>{}))
    {
        return transform_ref_fn::impl(std::forward<Tup>(tup),
                                      make_tuple_indices<Tup>{});
    }
};

// Transforms tuple of dereferencable to tuple of dereferenced values:
//   tuple<int*, boost::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int, double, short>
struct transform_deref_fn
{
private:
    template <typename Tup, std::size_t... Is>
    static auto impl(Tup&& tup, index_sequence<Is...>)
        -> decltype(std::make_tuple(*std::get<Is>(std::forward<Tup>(tup))...))
    {
        return std::make_tuple(*std::get<Is>(std::forward<Tup>(tup))...);
    }

public:
    template <typename Tup>
    static auto call(Tup&& tup)
        -> decltype(transform_deref_fn::impl(std::forward<Tup>(tup),
                                             make_tuple_indices<Tup>{}))
    {
        return transform_deref_fn::impl(std::forward<Tup>(tup),
                                        make_tuple_indices<Tup>{});
    }
};

// Increments each value of given tuple
struct increment_each_fn
{
private:
    template <typename Tup, std::size_t... Is>
    static void impl(Tup&& tup, index_sequence<Is...>)
    {
        using swallow = std::initializer_list<int>;
        (void)swallow{((void)++(std::get<Is>(std::forward<Tup>(tup))), 0)...};
    }

public:
    template <typename Tup>
    static void call(Tup&& tup)
    {
        impl(std::forward<Tup>(tup), make_tuple_indices<Tup>{});
    }
};

// Checks for inequality with two given tuples. We AND instead of OR to
// support zipping Sequences with different size.
struct not_equal_fn
{
private:
    template <typename Tup>
    static bool impl(Tup&&, Tup&&)
    {
        return true;
    }

    template <std::size_t I, std::size_t... Is, typename Tup>
    static bool impl(Tup&& tup0, Tup&& tup1)
    {
        return std::get<I>(std::forward<Tup>(tup0)) !=
                   std::get<I>(std::forward<Tup>(tup1)) &&
               not_equal_fn::template impl<Is...>(std::forward<Tup>(tup0),
                                                  std::forward<Tup>(tup1));
    }

    template <typename Tup, std::size_t... Is>
    static bool impl2(Tup&& tup0, Tup&& tup1, index_sequence<Is...>)
    {
        return not_equal_fn::impl<Is...>(std::forward<Tup>(tup0),
                                         std::forward<Tup>(tup1));
    }

public:
    template <typename Tup>
    static bool call(Tup&& tup0, Tup&& tup1)
    {
        return not_equal_fn::impl2(std::forward<Tup>(tup0),
                                   std::forward<Tup>(tup1),
                                   make_tuple_indices<Tup>{});
    }
};

// Calculates minimum value of std::distance's on each pair of the tuples
struct distance_fn
{
private:
#if defined(_MSC_VER)
    template <typename T>
    static const T& min(const T& arg)
    {
        return arg;
    }

    template <typename T0, typename T1, typename... Ts>
    static const typename std::common_type<T0, T1, Ts...>::type&
        min(const T0& arg1, const T1& arg2, const Ts&... args)
    {
        return (arg1 < arg2) ? min(arg1, args...) : min(arg2, args...);
    }

    // This is faster on MSVC2015 than foldl-like with an accumulator
    template <typename Tup, std::size_t... Is>
    static std::ptrdiff_t impl2(Tup&& tup0, Tup&& tup1, index_sequence<Is...>)
    {
        return distance_fn::min(
            std::distance(std::get<Is>(std::forward<Tup>(tup0)),
                          std::get<Is>(std::forward<Tup>(tup1)))...);
    }
#else
    template <typename Tup>
    static std::ptrdiff_t impl(Tup&&, Tup&&, std::ptrdiff_t acc)
    {
        return acc;
    }

    template <std::size_t I, std::size_t... Is, typename Tup>
    static std::ptrdiff_t impl(Tup&& tup0, Tup&& tup1, std::ptrdiff_t acc)
    {
        return distance_fn::template impl<Is...>(
            std::forward<Tup>(tup0), std::forward<Tup>(tup1),
            std::min(acc, std::distance(std::get<I>(std::forward<Tup>(tup0)),
                                        std::get<I>(std::forward<Tup>(tup1)))));
    }

    template <typename Tup, std::size_t... Is>
    static std::ptrdiff_t impl2(Tup&& tup0, Tup&& tup1, index_sequence<Is...>)
    {
        return distance_fn::impl<Is...>(
            std::forward<Tup>(tup0), std::forward<Tup>(tup1),
            std::numeric_limits<std::ptrdiff_t>::max());
    }
#endif

public:
    template <typename Tup>
    static std::ptrdiff_t call(Tup&& tup0, Tup&& tup1)
    {
        return distance_fn::impl2(std::forward<Tup>(tup0),
                                  std::forward<Tup>(tup1),
                                  make_tuple_indices<Tup>{});
    }
};

// Checks if all tuple elements are convertible to true
struct all_true_fn
{
private:
    template <typename Tup>
    static bool impl(Tup&&)
    {
        return true;
    }

    template <std::size_t I, std::size_t... Is, typename Tup>
    static bool impl(Tup&& tup)
    {
        return !!(std::get<I>(std::forward<Tup>(tup))) &&
               all_true_fn::template impl<Is...>(std::forward<Tup>(tup));
    }

    template <typename Tup, std::size_t... Is>
    static bool impl2(Tup&& tup, index_sequence<Is...>)
    {
        return all_true_fn::impl<Is...>(std::forward<Tup>(tup));
    }

public:
    template <typename Tup>
    static bool call(Tup&& tup)
    {
        return all_true_fn::impl2(std::forward<Tup>(tup),
                                  make_tuple_indices<Tup>{});
    }
};
} // namespace tuple
} // namespace kl
