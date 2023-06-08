#pragma once

#include "kl/type_traits.hpp"

#include <concepts>
#include <ranges>
#include <type_traits>

namespace kl::detail {

template <typename T>
concept growable_range = std::ranges::range<T> &&
    requires(T a) {
        typename T::value_type;
        requires requires (typename T::value_type x) {
            a.push_back(x);
        };
    };

template <typename T>
concept map_alike = std::ranges::range<T> &&
    requires(T a) {
        typename T::key_type;
        typename T::mapped_type;
    };

template <typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;
} // namespace kl::detail
