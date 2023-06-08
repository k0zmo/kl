#pragma once

#include <cstddef>
#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kl::tuple {

template <typename T>
using make_tuple_indices =
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>;

// Applies a function to given tuple
template <typename Tup, typename Fun>
decltype(auto) apply(Tup&& tup, Fun&& fun)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::forward<Fun>(fun)(std::get<Is>(std::forward<Tup>(tup))...);
    } (make_tuple_indices<Tup>{});
}

template <typename Tup, typename Fun>
void for_each(Tup&& tup, Fun&& fun)
{
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., fun(std::get<Is>(std::forward<Tup>(tup))));
    } (make_tuple_indices<Tup>{});
}

// Transforms tuple of dereferencable to tuple of references:
//   tuple<int*, std::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int&, double&, const short&>
template <typename Tup>
auto transform_ref(Tup&& tup)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::tuple<decltype(*std::get<Is>(std::forward<Tup>(
            tup)))...>{(*std::get<Is>(std::forward<Tup>(tup)))...};
    } (make_tuple_indices<Tup>{});
}

// Transforms tuple of dereferencable to tuple of dereferenced values:
//   tuple<int*, std::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int, double, short>
template <typename Tup>
auto transform_deref(Tup&& tup)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(
            (*std::get<Is>(std::forward<Tup>(tup)))...);
    } (make_tuple_indices<Tup>{});
}

// Checks for inequality with two given tuples. We AND instead of OR to
// support zipping Sequences with different size.
template <typename Tup1, typename Tup2>
bool not_equal(Tup1&& tup1, Tup2&& tup2)
{
    static_assert(std::tuple_size_v<std::remove_reference_t<Tup1>> ==
                    std::tuple_size_v<std::remove_reference_t<Tup2>>);

    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... && (std::get<Is>(std::forward<Tup1>(tup1)) !=
                        std::get<Is>(std::forward<Tup2>(tup2))));
    } (make_tuple_indices<Tup1>{});
}

// Calculates minimum value of std::distance's on each pair of the tuples
template <typename Tup>
std::ptrdiff_t distance(Tup&& tup0, Tup&& tup1)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
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

// Checks if all tuple elements are convertible to true
template <typename Tup>
bool all_true(Tup&& tup)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... && std::get<Is>(std::forward<Tup>(tup)));
    } (make_tuple_indices<Tup>{});
}

} // namespace kl::tuple
