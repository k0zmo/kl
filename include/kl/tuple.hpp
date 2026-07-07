#pragma once

#include <cstddef>
#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>

namespace kl {

using std::index_sequence;
using std::make_index_sequence;

template <typename T>
using make_tuple_indices =
    make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>;

namespace tuple {

struct for_each_fn
{
private:
    template <typename Tup, typename Fun, std::size_t... Is>
    static void impl(Tup&& tup, Fun&& fun, index_sequence<Is...>)
    {
        (..., fun(std::get<Is>(std::forward<Tup>(tup))));
    }

public:
    template <typename Tup, typename Fun>
    static void call(Tup&& tup, Fun&& fun)
    {
        for_each_fn::impl(std::forward<Tup>(tup), std::forward<Fun>(fun),
                          make_tuple_indices<Tup>{});
    }
};

// Transforms tuple of dereferencable to tuple of references:
//   tuple<int*, std::optional<double>, std::vector<short>::const_iterator> ->
//   tuple<int&, double&, const short&>
struct transform_ref_fn
{
private:
    template <typename Tup, std::size_t... Is>
    static auto impl(Tup&& tup, index_sequence<Is...>)
    {
        return std::tuple<decltype(*(std::get<Is>(std::forward<Tup>(tup))))...>{
            (*std::get<Is>(std::forward<Tup>(tup)))...};
    }

public:
    template <typename Tup>
    static auto call(Tup&& tup)
    {
        return transform_ref_fn::impl(std::forward<Tup>(tup),
                                      make_tuple_indices<Tup>{});
    }
};

// Checks for inequality with two given tuples. We AND instead of OR to
// support zipping Sequences with different size.
struct not_equal_fn
{
private:
    template <typename Tup1, typename Tup2, std::size_t... Is>
    static bool impl(Tup1&& tup1, Tup2&& tup2, index_sequence<Is...>)
    {
        return (... && (std::get<Is>(std::forward<Tup1>(tup1)) !=
                        std::get<Is>(std::forward<Tup2>(tup2))));
    }

public:
    template <typename Tup1, typename Tup2>
    static bool call(Tup1&& tup1, Tup2&& tup2)
    {
        static_assert(std::tuple_size_v<std::remove_reference_t<Tup1>> ==
                      std::tuple_size_v<std::remove_reference_t<Tup2>>);

        return not_equal_fn::impl(std::forward<Tup1>(tup1),
                                  std::forward<Tup2>(tup2),
                                  make_tuple_indices<Tup1>{});
    }
};

} // namespace tuple
} // namespace kl
