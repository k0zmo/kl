#pragma once

#include "kl/detail/macros.hpp"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

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
 * The rest is a list of all fields that need to be visible by kl::ctti.
   Field entries may be plain member names or tuples in the form
   (member, metadata...). Metadata is stored in the reflected field object and
   can be queried with field.has<T>() and field.get<T>().
 * Macro should be placed in the same namespace as the type
 * Above definitions are expanded to:

     template <typename Visitor, typename Self>
     constexpr void reflect_struct(Visitor&& vis, Self&& self,
                                   ::kl::ctti::record_class<A>)
     {
         vis(field object for i);
         vis(field object for b);
         vis(field object for d);
     }

     constexpr std::size_t reflect_num_fields(::kl::ctti::record_class<A>)
         noexcept
     {
         return 3;
     }

     template <typename Visitor, typename Self>
     constexpr void reflect_struct(Visitor&& vis, Self&& self,
                                   ::kl::ctti::record_class<B>)
     {
         reflect_struct(vis, self, ::kl::ctti::record<A>);
         vis(field object for str);
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

 * Use kl::ctti to query type's fields:
     ns::B b = ...
     kl::ctti::reflect(b, [](auto field) {
        // field.name()
        // field.value()
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

namespace detail {

// Shared storage and metadata support for reflected fields.
template <typename Object, typename... Attributes>
class field_base
{
    template <typename Query, typename Stored>
    inline constexpr static bool attribute_matches_v =
        std::is_same_v<Query, Stored> ||
        std::is_convertible_v<const Stored*, const Query*>;

public:
    constexpr field_base(Object& object, const char* name, Attributes... attributes)
        : object_{&object}, name_{name}, attributes_{std::move(attributes)...}
    {
    }

    constexpr const char* name() const noexcept
    {
        return name_;
    }

    template <typename Attribute>
    static constexpr bool has()
    {
        return (attribute_matches_v<Attribute, Attributes> || ...);
    }

    template <typename Attribute>
    constexpr const Attribute* get() const
    {
        return get_impl<Attribute, 0>();
    }

    template <typename Visitor>
    constexpr void visit_attributes(Visitor&& vis) const
    {
        std::apply([&](const auto&... attr) { (vis(attr), ...); }, attributes_);
    }

private:
    template <typename Attribute, std::size_t I>
    constexpr const Attribute* get_impl() const
    {
        if constexpr (I == sizeof...(Attributes))
        {
            return nullptr;
        }
        else
        {
            using current = std::tuple_element_t<I, std::tuple<Attributes...>>;

            if constexpr (attribute_matches_v<Attribute, current>)
            {
                return &std::get<I>(attributes_);
            }
            else
            {
                return get_impl<Attribute, I + 1>();
            }
        }
    }

protected:
    constexpr Object& object() const noexcept
    {
        return *object_;
    }

private:
    Object* object_;
    const char* name_;
    std::tuple<Attributes...> attributes_;
};

} // namespace detail

// Reflected direct data member.
template <auto Ptr, typename Object, typename... Attributes>
class field : public detail::field_base<Object, Attributes...>
{
public:
    using detail::field_base<Object, Attributes...>::field_base;

    constexpr decltype(auto) value() const { return ((this->object()).*Ptr); }
};

template <auto Ptr, typename Object, typename... Attributes>
constexpr auto make_field(Object& object, const char* name, Attributes... attributes)
{
    return field<Ptr, Object, Attributes...>{object, name, std::move(attributes)...};
}

// Reflected field backed by a callable accessor.
// Use this for cases that cannot be represented by a pointer-to-member, such as
// reference data members, flattened nested members, or computed/custom views.
template <typename Object, typename Accessor, typename... Attributes>
class accessor_field : public detail::field_base<Object, Attributes...>
{
public:
    constexpr accessor_field(Object& object, const char* name, Accessor accessor,
                             Attributes... attributes)
        : detail::field_base<Object, Attributes...>{object, name, std::move(attributes)...},
          accessor_{std::move(accessor)}
    {
    }

    constexpr decltype(auto) value() const { return accessor_(this->object()); }

private:
    Accessor accessor_;
};

template <typename Object, typename Accessor, typename... Attributes>
accessor_field(Object&, const char*, Accessor, Attributes...)
    -> accessor_field<Object, Accessor, Attributes...>;

} // namespace kl::ctti

#define KL_REFLECT_STRUCT(type_, ...)                                          \
    KL_REFLECT_STRUCT_TUPLE(type_, KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_DERIVED(type_, bases_, ...)                          \
    KL_REFLECT_STRUCT_DERIVED_TUPLE(type_, KL_ARG_TO_TUPLE(bases_),            \
                                    KL_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_REFLECT_STRUCT_TUPLE(type_, fields_)                                \
    template <typename Visitor, typename Self>                                 \
    [[maybe_unused]] constexpr void reflect_struct(                            \
        Visitor&& vis, Self&& self, ::kl::ctti::record_class<type_>)           \
    {                                                                          \
        KL_REFLECT_STRUCT_VIS_MEMBERS(type_, fields_)                          \
    }                                                                          \
                                                                               \
    [[maybe_unused]] constexpr std::size_t reflect_num_fields(                 \
        ::kl::ctti::record_class<type_>) noexcept                              \
    {                                                                          \
        return KL_TUPLE_SIZE(fields_);                                         \
    }

#define KL_REFLECT_STRUCT_DERIVED_TUPLE(type_, bases_, fields_)                \
    template <typename Visitor, typename Self>                                 \
    [[maybe_unused]] constexpr void reflect_struct(                            \
        Visitor&& vis, Self&& self, ::kl::ctti::record_class<type_>)           \
    {                                                                          \
        KL_REFLECT_STRUCT_VIS_BASES(bases_)                                    \
        KL_REFLECT_STRUCT_VIS_MEMBERS(type_, fields_)                          \
    }                                                                          \
                                                                               \
    [[maybe_unused]] constexpr std::size_t reflect_num_fields(                 \
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
    vis(::kl::ctti::make_field<&type_::KL_TUPLE_ELEM(0, field_)>(              \
        self, KL_STRINGIZE(KL_TUPLE_ELEM(0, field_))                           \
                  KL_TUPLE_ENUM_POP_FRONT_COMMA(field_)));

#define KL_REFLECT_STRUCT_VIS_BASE(base_)                                      \
    reflect_struct(vis, self, ::kl::ctti::record<base_>);

#define KL_REFLECT_STRUCT_NUM_FIELDS(base_)                                    \
    reflect_num_fields(::kl::ctti::record<base_>) +

#define KL_ACCESSOR_FIELD(name_)                                               \
    KL_ACCESSOR_FIELD_NAMED(KL_STRINGIZE(name_), object.name_)

#define KL_ACCESSOR_FIELD_NAMED(name_, expression_)                            \
    ::kl::ctti::accessor_field                                                 \
    {                                                                          \
        self, name_,                                                           \
            [](auto& object) -> decltype(auto) { return (expression_); }       \
    }
