#pragma once

#include "kl/type_traits.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace kl::serialization {

template <typename T>
struct serializer;

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

template <typename T>
bool is_null_value(const T& t)
{
    return optional_traits<T>::is_null_value(t);
}

namespace detail {

KL_VALID_EXPR_HELPER(has_empty, std::declval<const T&>().empty())

} // namespace detail

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

template <typename Value>
struct backend_for_value;

namespace detail {

template <typename BackendIdentity>
using backend_t = typename BackendIdentity::backend_type;

template <typename Value>
using backend_for_value_t =
    typename backend_for_value<remove_cvref_t<Value>>::type;

} // namespace detail

} // namespace kl::serialization
