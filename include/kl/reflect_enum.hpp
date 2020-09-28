#pragma once

#include "kl/detail/macro.hpp"
#include "kl/range.hpp"

/*
 * Requirements: boost 1.57+, C++14 compiler and preprocessor
 * Sample usage:

namespace ns {

enum class enum_
{
    A, B, C
};
KL_REFLECT_ENUM(enum_, A, (B, bb), C)
}

 * First argument is unqualified enum type name
 * The rest is a list of enum values. Each value can be optionally a
   tuple (pair) of its real name and name used for to-from string conversions
 * Macro should be placed inside the same namespace as the enum type

 * Use kl::enum_reflector<ns::enum_> to query enum properties:
     kl::enum_reflector<ns::enum_>::to_string(ns::enum_{C});
     kl::enum_reflector<ns::enum_>::from_string("bb");
     kl::enum_reflector<ns::enum_>::count();
     kl::enum_reflector<ns::enum_>::values();
 * Alternatively, use kl::reflect<ns::enum_>()

 * Remarks: Macro KL_REFLECT_ENUM works for unscoped as well as scoped
   enums

 * Above definition is expanded to:

     static constexpr ::kl::enum_value_name<enum_> kl_enum_description356[] = {
         {enum_::A, "A"}, {enum_::B, "bb"}, {enum_::C, "C"}};
     constexpr auto reflect_enum(::kl::enum_class<enum_>) noexcept
     {
         return ::kl::range{kl_enum_description356};
     }
 */

namespace kl {

// clang-format off
template <typename Enum>
class enum_class {};

template <typename Enum>
inline constexpr auto enum_ = enum_class<Enum>{};
// clang-format on

template <typename Enum>
struct enum_value_name
{
    Enum value;
    const char* name;
};
} // namespace kl

#define KL_REFLECT_ENUM(name_, ...)                                            \
    KL_REFLECT_ENUM_TUPLE(name_, KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_ENUM_TUPLE(name_, values_)                                  \
    KL_REFLECT_ENUM_IMPL(name_, values_, __COUNTER__)

#define KL_REFLECT_ENUM_IMPL(name_, values_, counter_)                         \
    inline constexpr ::kl::enum_value_name<name_> KL_REFLECT_ENUM_VAR_NAME(    \
        counter_)[] = {KL_REFLECT_ENUM_VALUE_NAME_PAIRS(name_, values_)};      \
    constexpr auto reflect_enum(::kl::enum_class<name_>) noexcept              \
    {                                                                          \
        return ::kl::range{KL_REFLECT_ENUM_VAR_NAME(counter_)};                \
    }

#define KL_REFLECT_ENUM_VAR_NAME(counter_)                                     \
    KL_CONCAT(kl_enum_description, counter_)

#define KL_REFLECT_ENUM_VALUE_NAME_PAIRS(name_, values_)                       \
    KL_TUPLE_FOR_EACH2(name_, values_, KL_REFLECT_ENUM_VALUE_NAME_PAIR)

#define KL_REFLECT_ENUM_VALUE_NAME_PAIR(name_, value_)                         \
    KL_REFLECT_ENUM_VALUE_NAME_PAIR2(name_, KL_ARG_TO_TUPLE(value_))

// Assumes value_ is a tuple: (x) or (x, y)
#define KL_REFLECT_ENUM_VALUE_NAME_PAIR2(name_, value_)                        \
    {name_::KL_TUPLE_ELEM(0, value_),                                          \
     KL_STRINGIZE(KL_TUPLE_SECOND_OR_FIRST_ELEM(value_))},
