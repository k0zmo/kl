#pragma once

#include "kl/type_traits.hpp"

namespace kl {

// Adds T to front of TypePack
template <typename T, typename TypePack>
struct push_front;

template <typename T, typename... Args>
struct push_front<T, kl::type_pack<Args...>>
{
    using type = kl::type_pack<T, Args...>;
};

// Filters out types that don't satisfy given Predicate<T> from given type pack
template <template <class> class Predicate, typename...>
struct filter;

template <template <class> class Predicate>
struct filter<Predicate>
{
    using type = kl::type_pack<>;
};

template <template <class> class Predicate, typename Head, typename... Tail>
struct filter<Predicate, Head, Tail...>
{
    using type = std::conditional_t<
        Predicate<Head>::value,
        typename push_front<Head,
                            typename filter<Predicate, Tail...>::type>::type,
        typename filter<Predicate, Tail...>::type>;
};
} // namespace kl
