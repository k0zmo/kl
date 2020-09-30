#pragma once

#include "kl/type_traits.hpp"

#include <type_traits>

namespace kl::detail {

KL_HAS_TYPEDEF_HELPER(value_type)
KL_HAS_TYPEDEF_HELPER(iterator)
KL_HAS_TYPEDEF_HELPER(mapped_type)
KL_HAS_TYPEDEF_HELPER(key_type)

KL_VALID_EXPR_HELPER(has_reserve, std::declval<T&>().reserve(0U))

template <typename T>
struct is_map_alike
    : std::conjunction<
        has_value_type<T>,
        has_iterator<T>,
        has_mapped_type<T>,
        has_key_type<T>> {};

template <typename T>
struct is_vector_alike
    : std::conjunction<
        has_value_type<T>,
        has_iterator<T>> {};

} // namespace kl::detail
