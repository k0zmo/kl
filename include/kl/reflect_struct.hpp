#pragma once

#include "kl/detail/macros.hpp"

#include <cstddef> // IWYU pragma: keep

namespace kl::ctti {

/*
 * Requirements: boost 1.61+, C++17 compiler and preprocessor
 * Sample usage:

namespace ns {

struct A
{
    int i;
    bool b;
    double d;
};
KL_REFLECT_STRUCT(A, i, b, d)
// or, inside the struct definition:
// KL_REFLECT_STRUCT_FRIEND(A, i, b, d)

struct B : A
{
    std::string str;
};
KL_REFLECT_STRUCT_DERIVED(B, A, str)
// or KL_REFLECT_STRUCT_DERIVED(B, (A), str)
// or, inside the struct definition:
// KL_REFLECT_STRUCT_DERIVED_FRIEND(B, A, str)

}

 KL_REFLECT_STRUCT
 =================

 * First argument is unqualified type name
 * The rest is a list of all fields that need to be visible by kl::ctti.
   Field entries may be plain member names or tuples in the form
   (member, attributes...). Attributes are attached to fields produced later by
   ctti.hpp factories and can be queried with field.has<T>() and field.get<T>().
 * Macro should be placed in the same namespace as the type
 * Alternatively, KL_REFLECT_STRUCT_FRIEND can be placed inside the type.
   This is useful when reflected members are private or protected.
 * This header only declares the field list. The actual runtime fields,
   object-free field descriptors, factories, reflect_object(), and reflect_type()
   live in kl/ctti.hpp.
 * Above definitions are expanded roughly to:

     template <typename Factory, typename Visitor>
     constexpr void reflect_struct_fields(
         Factory&& factory, Visitor&& vis, ::kl::ctti::record_class<A>)
     {
         vis(factory.template field<&A::i>("i"));
         vis(factory.template field<&A::b>("b"));
         vis(factory.template field<&A::d>("d"));
     }

     constexpr std::size_t reflect_num_fields(::kl::ctti::record_class<A>)
         noexcept
     {
         return 3;
     }

     template <typename Factory, typename Visitor>
     constexpr void reflect_struct_fields(
         Factory&& factory, Visitor&& vis, ::kl::ctti::record_class<B>)
     {
         reflect_struct_fields(factory, vis, ::kl::ctti::record<A>);
         vis(factory.template field<&B::str>("str"));
     }

     constexpr std::size_t reflect_num_fields(::kl::ctti::record_class<B>)
         noexcept
     {
         return 1 + reflect_num_fields(::kl::ctti::record<A>) + 0;
     }

 KL_REFLECT_STRUCT_DERIVED
 =========================

 * First argument is unqualified type name
 * Second argument is a tuple of all direct base classes. In case of one base
   class parentheses can be omitted
 * The rest is a list of all fields that need to be visible by kl::ctti
 * Similarly, macro should be placed in the same namespace as the type
 * Alternatively, KL_REFLECT_STRUCT_DERIVED_FRIEND can be placed inside the type.

 * Use kl::ctti to query type's fields:
     #include "kl/ctti.hpp"

     ns::B b = ...
     kl::ctti::reflect_object(b, [](auto field) {
        // field.name()
        // field.value()
        // Called four times, once for each field
     });

     kl::ctti::reflect_type<ns::B>([](auto field) {
        // field.name()
        // typename decltype(field)::value_type
        // no field.value(), because no object is bound
     });

     static_assert(kl::ctti::num_fields<ns::B>() == 4);
 */

// clang-format off
template <typename Record>
class record_class {};

template <typename Record>
inline constexpr auto record = record_class<Record>{};
// clang-format on

} // namespace kl::ctti

#define KL_REFLECT_STRUCT(type_, ...)                                          \
    KL_REFLECT_STRUCT_TUPLE(type_, KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_FRIEND(type_, ...)                                   \
    KL_REFLECT_STRUCT_FRIEND_TUPLE(type_, KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_DERIVED(type_, bases_, ...)                          \
    KL_REFLECT_STRUCT_DERIVED_TUPLE(type_, KL_ARG_TO_TUPLE(bases_),            \
                                    KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_DERIVED_FRIEND(type_, bases_, ...)                   \
    KL_REFLECT_STRUCT_DERIVED_FRIEND_TUPLE(                                    \
        type_, KL_ARG_TO_TUPLE(bases_), KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_NAMESPACE_DECL [[maybe_unused]]
#define KL_REFLECT_STRUCT_FRIEND_DECL friend

#define KL_REFLECT_STRUCT_TUPLE(type_, fields_)                                \
    KL_REFLECT_STRUCT_TUPLE_IMPL(KL_REFLECT_STRUCT_NAMESPACE_DECL, type_,      \
                                 fields_)

#define KL_REFLECT_STRUCT_FRIEND_TUPLE(type_, fields_)                         \
    KL_REFLECT_STRUCT_TUPLE_IMPL(KL_REFLECT_STRUCT_FRIEND_DECL, type_, fields_)

#define KL_REFLECT_STRUCT_TUPLE_IMPL(decl_, type_, fields_)                    \
    template <typename Factory, typename Visitor>                              \
    decl_ constexpr void reflect_struct_fields(                                \
        Factory&& factory, Visitor&& vis, ::kl::ctti::record_class<type_>)     \
    {                                                                          \
        KL_REFLECT_STRUCT_VIS_MEMBERS(type_, fields_)                          \
    }                                                                          \
                                                                               \
    decl_ constexpr std::size_t reflect_num_fields(                            \
        ::kl::ctti::record_class<type_>) noexcept                              \
    {                                                                          \
        return KL_TUPLE_SIZE(fields_);                                         \
    }

#define KL_REFLECT_STRUCT_DERIVED_TUPLE(type_, bases_, fields_)                \
    KL_REFLECT_STRUCT_DERIVED_TUPLE_IMPL(KL_REFLECT_STRUCT_NAMESPACE_DECL,     \
                                         type_, bases_, fields_)

#define KL_REFLECT_STRUCT_DERIVED_FRIEND_TUPLE(type_, bases_, fields_)         \
    KL_REFLECT_STRUCT_DERIVED_TUPLE_IMPL(KL_REFLECT_STRUCT_FRIEND_DECL, type_, \
                                         bases_, fields_)

#define KL_REFLECT_STRUCT_DERIVED_TUPLE_IMPL(decl_, type_, bases_, fields_)    \
    template <typename Factory, typename Visitor>                              \
    decl_ constexpr void reflect_struct_fields(                                \
        Factory&& factory, Visitor&& vis, ::kl::ctti::record_class<type_>)     \
    {                                                                          \
        KL_REFLECT_STRUCT_VIS_BASES(bases_)                                    \
        KL_REFLECT_STRUCT_VIS_MEMBERS(type_, fields_)                          \
    }                                                                          \
                                                                               \
    decl_ constexpr std::size_t reflect_num_fields(                            \
        ::kl::ctti::record_class<type_>) noexcept                              \
    {                                                                          \
        return KL_TUPLE_SIZE(fields_) +                                        \
               KL_REFLECT_STRUCT_NUM_BASE_FIELDS(bases_) 0;                    \
    }

#define KL_REFLECT_STRUCT_VIS_MEMBERS(type_, fields_)                          \
    KL_TUPLE_FOR_EACH2(type_, fields_, KL_REFLECT_STRUCT_VIS_MEMBER)

#define KL_REFLECT_STRUCT_VIS_BASES(fields_)                                   \
    KL_TUPLE_FOR_EACH(fields_, KL_REFLECT_STRUCT_VIS_BASE)

#define KL_REFLECT_STRUCT_NUM_BASE_FIELDS(fields_)                             \
    KL_TUPLE_FOR_EACH(fields_, KL_REFLECT_STRUCT_NUM_FIELDS)

#define KL_REFLECT_STRUCT_VIS_MEMBER(type_, name_)                             \
    KL_REFLECT_STRUCT_VIS_MEMBER_IMPL(type_, KL_ARG_TO_TUPLE(name_))

#define KL_REFLECT_STRUCT_VIS_MEMBER_IMPL(type_, field_)                       \
    KL_REFLECT_STRUCT_VIS_MEMBER_TUPLE(type_, field_)

#define KL_REFLECT_STRUCT_VIS_MEMBER_TUPLE(type_, field_)                      \
    vis(factory.template field<&type_::KL_TUPLE_ELEM(0, field_)>(              \
        KL_STRINGIZE(KL_TUPLE_ELEM(0, field_))                                 \
            KL_TUPLE_ENUM_POP_FRONT_LAMBDA_COMMA(field_)));

#define KL_REFLECT_STRUCT_VIS_BASE(base_)                                      \
    reflect_struct_fields(factory, vis, ::kl::ctti::record<base_>);

#define KL_REFLECT_STRUCT_NUM_FIELDS(base_)                                    \
    reflect_num_fields(::kl::ctti::record<base_>) +

#define KL_ACCESSOR_FIELD(name_)                                               \
    KL_ACCESSOR_FIELD_NAMED(KL_STRINGIZE(name_), object.name_)

#define KL_ACCESSOR_FIELD_NAMED(name_, expression_)                            \
    factory.accessor(                                                          \
        name_, [](auto& object) -> decltype(auto) { return (expression_); })
