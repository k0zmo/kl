#pragma once

#include "kl/type_traits.hpp"

#include <optional>
#include <utility>

namespace kl::serialization {

// Primary template for user-defined serialization logic.
template <typename T>
struct serializer;

// Trait used to determine whether a value should be considered "null".
template <typename T>
struct optional_traits
{
    static bool is_null_value(const T&) { return false; }
};

template <typename T>
struct optional_traits<std::optional<T>>
{
    static bool is_null_value(const std::optional<T>& opt) { return !opt; }
};

// Returns true when the value is considered null according to optional_traits.
template <typename T>
bool is_null_value(const T& t)
{
    return optional_traits<T>::is_null_value(t);
}

namespace detail {

KL_VALID_EXPR_HELPER(has_empty, std::declval<const T&>().empty())

} // namespace detail

// Trait used to determine whether a value should be considered "empty".
// Works automatically for types that provide an empty() member
template <typename T>
struct empty_traits
{
    static bool is_empty_value(const T& value)
    {
        if constexpr (detail::has_empty_v<T>)
        {
            return value.empty();
        }
        else
        {
            static_assert(always_false_v<T>,
                          "serialization empty_traits<T> must be specialized "
                          "or T must provide empty()");
        }
    }
};

// Returns true when the value is considered empty according to empty_traits.
template <typename T>
bool is_empty_value(const T& t)
{
    return empty_traits<T>::is_empty_value(t);
}

template <typename Backend>
struct backend_traits;

template <typename Backend>
struct backend_tag
{
    using backend_type = Backend;
};

namespace detail {

template <typename BackendIdentity>
using backend_t = typename BackendIdentity::backend_type;

} // namespace detail

} // namespace kl::serialization
