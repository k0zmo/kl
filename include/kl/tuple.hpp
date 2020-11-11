#pragma once

#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kl {

using std::index_sequence;
using std::make_index_sequence;

template <typename T>
using make_tuple_indices =
    make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>;

namespace tuple {

// Applies a function to given tuple
struct apply_fn
{
    template <typename Tup, typename Fun>
    static decltype(auto) call(Tup&& tup, Fun&& fun)
    {
        return [&]<std::size_t... Is>(index_sequence<Is...>) {
            return std::forward<Fun>(fun)(std::get<Is>(std::forward<Tup>(tup))...);
        } (make_tuple_indices<Tup>{});
    }
};

struct for_each_fn
{
    template <typename Tup, typename Fun>
    static void call(Tup&& tup, Fun&& fun)
    {
        [&]<std::size_t... Is>(index_sequence<Is...>) {
            (..., fun(std::get<Is>(std::forward<Tup>(tup))));
        } (make_tuple_indices<Tup>{});
    }
};

// Transforms tuple of dereferencable to tuple of references:
//   tuple<int*, std::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int&, double&, const short&>
struct transform_ref_fn
{
    template <typename Tup>
    static auto call(Tup&& tup)
    {
        return [&]<std::size_t... Is>(index_sequence<Is...>) {
            return std::tuple<decltype(*std::get<Is>(std::forward<Tup>(
                tup)))...>{(*std::get<Is>(std::forward<Tup>(tup)))...};
        } (make_tuple_indices<Tup>{});
    }
};

// Transforms tuple of dereferencable to tuple of dereferenced values:
//   tuple<int*, std::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int, double, short>
struct transform_deref_fn
{
    template <typename Tup>
    static auto call(Tup&& tup)
    {
        return [&]<std::size_t... Is>(index_sequence<Is...>) {
            return std::make_tuple(
                (*std::get<Is>(std::forward<Tup>(tup)))...);
        } (make_tuple_indices<Tup>{});
    }
};

// Checks for inequality with two given tuples. We AND instead of OR to
// support zipping Sequences with different size.
struct not_equal_fn
{
    template <typename Tup1, typename Tup2>
    static bool call(Tup1&& tup1, Tup2&& tup2)
    {
        static_assert(std::tuple_size_v<std::remove_reference_t<Tup1>> ==
                      std::tuple_size_v<std::remove_reference_t<Tup2>>);

        return [&]<std::size_t... Is>(index_sequence<Is...>) {
            return (... && (std::get<Is>(std::forward<Tup1>(tup1)) !=
                            std::get<Is>(std::forward<Tup2>(tup2))));
        } (make_tuple_indices<Tup1>{});
    }
};

// Calculates minimum value of std::distance's on each pair of the tuples
struct distance_fn
{
    template <typename Tup>
    static std::ptrdiff_t call(Tup&& tup0, Tup&& tup1)
    {
        return [&]<std::size_t... Is>(index_sequence<Is...>) {
            static_assert(sizeof...(Is) > 0);

            std::ptrdiff_t min{std::numeric_limits<std::ptrdiff_t>::max()},
                dist{};

            return ((min = (dist = std::distance(
                                std::get<Is>(std::forward<Tup>(tup0)),
                                std::get<Is>(std::forward<Tup>(tup1))),
                            dist < min ? dist : min)),
                    ...);
        } (make_tuple_indices<Tup>{});
    }
};

// Checks if all tuple elements are convertible to true
struct all_true_fn
{
    template <typename Tup>
    static bool call(Tup&& tup)
    {
        return [&]<std::size_t... Is>(index_sequence<Is...>) {
            return (... && std::get<Is>(std::forward<Tup>(tup)));
        } (make_tuple_indices<Tup>{});
    }
};
} // namespace tuple
} // namespace kl
