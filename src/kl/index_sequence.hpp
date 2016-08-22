#pragma once

#include <type_traits>
#include <tuple>

namespace kl {

#if (defined(_MSC_VER) && _MSC_VER < 1900) ||                                  \
    (!defined(_MSC_VER) && __cplusplus < 201402)
template <std::size_t... I>
struct index_sequence {};

template <std::size_t N, std::size_t... I>
struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, I...>
{};

template <std::size_t... I>
struct make_index_sequence_impl<0, I...>
{
    using type = index_sequence<I...>;
};

template <std::size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

#else
using std::make_index_sequence;
using std::index_sequence;
#endif
} // namespace kl
