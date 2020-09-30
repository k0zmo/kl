#pragma once

#include "kl/type_traits.hpp"

#include <type_traits>

namespace kl::detail {

KL_HAS_TYPEDEF_HELPER(value_type)
KL_HAS_TYPEDEF_HELPER(mapped_type)
KL_HAS_TYPEDEF_HELPER(key_type)

KL_VALID_EXPR_HELPER(
    has_push_back,
    std::declval<T&>().push_back(std::declval<typename T::value_type&>()))
KL_VALID_EXPR_HELPER(has_reserve, std::declval<T&>().reserve(0U))
KL_VALID_EXPR_HELPER(has_begin, std::declval<const T&>().begin())
KL_VALID_EXPR_HELPER(has_end, std::declval<const T&>().end())

template <typename T>
struct is_range
    : std::conjunction<
        has_begin<T>,
        has_end<T>> {};

template <typename T>
struct is_growable_range
    : std::conjunction<
        is_range<T>,
        has_value_type<T>,
        has_push_back<T>> {};

template <typename T>
struct is_map_alike
    : std::conjunction<
        is_range<T>,
        has_mapped_type<T>,
        has_key_type<T>> {};

} // namespace kl::detail
