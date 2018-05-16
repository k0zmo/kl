#pragma once

#include "kl/type_traits.hpp"

namespace kl {

// Adds T to the front of TypePack
template <typename T, typename TypePack>
struct push_front;

template <typename T, typename... Args>
struct push_front<T, type_pack<Args...>>
{
    using type = type_pack<T, Args...>;
};

template <typename Item, typename List>
using push_front_t = typename push_front<Item, List>::type;

// Filters out types that don't satisfy given Predicate<T> from given type pack
template <template <class> class Predicate, typename...>
struct filter;

template <template <class> class Predicate, typename... List>
using filter_t = typename filter<Predicate, List...>::type;

template <template <class> class Predicate>
struct filter<Predicate>
{
    using type = type_pack<>;
};

template <template <class> class Predicate, typename Head, typename... Tail>
struct filter<Predicate, Head, Tail...>
{
    using type =
        std::conditional_t<Predicate<Head>::value,
                           push_front_t<Head, filter_t<Predicate, Tail...>>,
                           filter_t<Predicate, Tail...>>;
};

// Unwrap type_pack and perform filter<>
template <template <class> class Predicate, typename... List>
struct filter<Predicate, type_pack<List...>>
{
    using type = filter_t<Predicate, List...>;
};
} // namespace kl
