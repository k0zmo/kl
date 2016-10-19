#pragma once

#include "kl/type_traits.hpp"
#include "kl/ctti.hpp"

namespace kl {
namespace type_class {

struct unknown {};
struct boolean {};
struct integral {};
struct floating {};
struct enumeration {};
struct string {};
struct reflectable {};
struct tuple {};
struct optional {};
struct vector {};
struct map {};

// Default traits
template <typename T>
struct is_bool : std::false_type {};

template <>
struct is_bool<bool> : std::true_type {};

template <typename T>
struct is_string : std::false_type {};

template <std::size_t N>
struct is_string<char[N]> : std::true_type {};

template <typename T>
struct is_tuple : std::false_type {};

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_vector: std::false_type {};

template <typename T>
struct is_map : std::false_type {};

template <typename T>
using get = std::conditional_t<
    is_bool<T>::value, type_class::boolean,
    std::conditional_t<
        std::is_integral<T>::value, type_class::integral,
        std::conditional_t<
            std::is_floating_point<T>::value, type_class::floating,
            std::conditional_t<
                std::is_enum<T>::value, type_class::enumeration,
                std::conditional_t<
                    is_string<T>::value, type_class::string,
                    std::conditional_t<
                        is_vector<T>::value, type_class::vector,
                        std::conditional_t<
                            is_reflectable<T>::value, type_class::reflectable,
                            std::conditional_t<
                                is_optional<T>::value, type_class::optional,
                                std::conditional_t<
                                    is_tuple<T>::value, type_class::tuple,
                                    std::conditional_t<is_map<T>::value,
                                                       type_class::map,
                                                       unknown>>>>>>>>>>;

template <typename T, typename TypeClass>
struct equals : std::is_same<type_class::get<T>, TypeClass> {};
} // namespace type_class
} // namespace kl

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <unordered_map>

namespace kl {
namespace type_class {

template <typename Char, typename CharTraits, typename Allocator>
struct is_string<std::basic_string<Char, CharTraits, Allocator>>
    : std::true_type {};

template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};

template <typename T, typename Allocator>
struct is_vector<std::vector<T, Allocator>> : std::true_type {};

template <typename Key, class Value, typename Compare, typename Allocator>
struct is_map<std::map<Key, Value, Compare, Allocator>> : std::true_type {};

template <typename Key, class Value, typename Hash, typename KeyEqual,
          typename Allocator>
struct is_map<std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>>
    : std::true_type {};
} // namespace type_class
} // namespace kl


#include <boost/optional.hpp>

namespace kl {
namespace type_class {

template <typename T>
struct is_optional<boost::optional<T>> : std::true_type {};
} // namespace type_class
} // namespace kl
