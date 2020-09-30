#pragma once

#include "kl/detail/macros.hpp"

#include <cstddef>

namespace kl {

/*
 * Requirements: boost 1.57+, C++14 compiler and preprocessor
 * Sample usage:

namespace ns {

struct A
{
    int i;
    bool b;
    double d;
};
KL_REFLECT_STRUCT(A, i, b, d)

struct B : A
{
    std::string str;
};
KL_REFLECT_STRUCT_DERIVED(B, A, str)
// or KL_REFLECT_STRUCT_DERIVED(B, (A), str)

}

 KL_REFLECT_STRUCT
 ==================

 * First argument is unqualified type name
 * The rest is a list of all fields that need to be visible by kl::ctti
 * Macro should be placed in the same namespace as the type
 * Above definitions are expanded to:

     template <typename Visitor, typename Self>
     constexpr void reflect_struct(Visitor&& vis, Self&& self,
                                   ::kl::record_class<A>)
     {
         vis(self.i, "i");
         vis(self.b, "b");
         vis(self.d, "d");
     }

     constexpr std::size_t reflect_num_fields(::kl::record_class<A>) noexcept
     {
         return 3;
     }

     template <typename Visitor, typename Self>
     constexpr void reflect_struct(Visitor&& vis, Self&& self,
                                   ::kl::record_class<B>)
     {
         reflect_struct(vis, self, ::kl::record<A>);
         vis(self.str, "str");
     }

     constexpr std::size_t reflect_num_fields(::kl::record_class<B>) noexcept
     {
         return 1 + reflect_num_fields(::kl::record<A>) + 0;
     }

 KL_REFLECT_STRUCT_DERIVED
 =========================

 * First argument is unqualified type name
 * Second argument is a tuple of all direct base classes. In case of one base
   class parentheses can be omitted
 * The rest is a list of all fields that need to be visible by kl::ctti
 * Similarly, macro should be placed in the same namespace as the type

 * Use kl::ctti to query type's fields:
     ns::B b = ...
     kl::ctti::reflect(b, [](auto& field, auto name) {
        // Called four times, once for each field
     });
     static_assert(kl::ctti::num_fields<ns::B>() == 4);
 */

// clang-format off
template <typename Record>
class record_class {};

template <typename Record>
inline constexpr auto record = record_class<Record>{};
// clang-format on

} // namespace kl

#define KL_REFLECT_STRUCT(type_, ...)                                          \
    KL_REFLECT_STRUCT_TUPLE(type_, KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_DERIVED(type_, bases_, ...)                          \
    KL_REFLECT_STRUCT_DERIVED_TUPLE(type_, KL_ARG_TO_TUPLE(bases_),            \
                                    KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_TUPLE(type_, fields_)                                \
    template <typename Visitor, typename Self>                                 \
    [[maybe_unused]] constexpr void reflect_struct(Visitor&& vis, Self&& self, \
                                                   ::kl::record_class<type_>)  \
    {                                                                          \
        KL_REFLECT_STRUCT_VIS_MEMBERS(fields_)                                 \
    }                                                                          \
                                                                               \
    [[maybe_unused]] constexpr std::size_t reflect_num_fields(                 \
        ::kl::record_class<type_>) noexcept                                    \
    {                                                                          \
        return KL_TUPLE_SIZE(fields_);                                         \
    }

#define KL_REFLECT_STRUCT_DERIVED_TUPLE(type_, bases_, fields_)                \
    template <typename Visitor, typename Self>                                 \
    [[maybe_unused]] constexpr void reflect_struct(Visitor&& vis, Self&& self, \
                                                   ::kl::record_class<type_>)  \
    {                                                                          \
        KL_REFLECT_STRUCT_VIS_BASES(bases_)                                    \
        KL_REFLECT_STRUCT_VIS_MEMBERS(fields_)                                 \
    }                                                                          \
                                                                               \
    [[maybe_unused]] constexpr std::size_t reflect_num_fields(                 \
        ::kl::record_class<type_>) noexcept                                    \
    {                                                                          \
        return KL_TUPLE_SIZE(fields_) +                                        \
               KL_REFLECT_STRUCT_NUM_BASE_FIELDS(bases_) 0;                    \
    }

#define KL_REFLECT_STRUCT_VIS_MEMBERS(fields_)                                 \
    KL_TUPLE_FOR_EACH(fields_, KL_REFLECT_STRUCT_VIS_MEMBER)

#define KL_REFLECT_STRUCT_VIS_BASES(fields_)                                   \
    KL_TUPLE_FOR_EACH(fields_, KL_REFLECT_STRUCT_VIS_BASE)

#define KL_REFLECT_STRUCT_NUM_BASE_FIELDS(fields_)                             \
    KL_TUPLE_FOR_EACH(fields_, KL_REFLECT_STRUCT_NUM_FIELDS)

#define KL_REFLECT_STRUCT_VIS_MEMBER(name_)                                    \
    vis(self.name_, KL_STRINGIZE(name_));

#define KL_REFLECT_STRUCT_VIS_BASE(base_)                                      \
    reflect_struct(vis, self, ::kl::record<base_>);

#define KL_REFLECT_STRUCT_NUM_FIELDS(base_)                                    \
    reflect_num_fields(::kl::record<base_>) +
