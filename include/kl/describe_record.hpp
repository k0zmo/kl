#pragma once

#include "kl/utility.hpp"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>

#include <tuple>
#include <type_traits>

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
KL_DESCRIBE_FIELDS(A, i, b, d)

struct B : A
{
    std::string str;
};
KL_DESCRIBE_BASES(B, A)
KL_DESCRIBE_FIELDS(B, str)
}

 KL_DESCRIBE_FIELDS
 ==================

 * First argument is unqualified type name
 * The rest is a list of all fields that need to be visible by kl::ctti
 * Macro should be placed in the same namespace as the type
 * Above definitions are expanded to:

     template <typename Self>
     constexpr auto describe_fields(A*, Self&& self) noexcept
     {
         return std::make_tuple(
            ::kl::make_field_info<decltype(self.i)>(self, self.i, "i"),
            ::kl::make_field_info<decltype(self.b)>(self, self.b, "b"),
            ::kl::make_field_info<decltype(self.d)>(self, self.d, "d"));
     }

     template <typename Self>
     constexpr auto describe_fields(B*, Self&& self) noexcept
     {
         return std::make_tuple(
            ::kl::make_field_info<decltype(self.str)>(self, self.str, "str"));
     }

 KL_DESCRIBE_BASES
 =================

 * First argument is unqualified type name
 * The rest is a list of all _direct_ base classes of the type
 * Similarly, macro should be placed in the same namespace as the type
 * Above definition is expanded to:

     constexpr auto describe_bases(B*) noexcept
     {
         return ::kl::type_pack<A>{};
     }


 * Use kl::ctti to query type's fields and base class(es):
     ns::B b = ...
     kl::ctti::reflect(b, [](auto fi) {
        // Called four times, once for each field
        // Use fi.name() and/or fi.get() to get a name
        // and reference to the field
     });
     static_assert(kl::ctti::num_fields<ns::B>() == 1, "?");
     static_assert(kl::ctti::total_num_fields<ns::B>() == 4, "?");
 */

namespace kl {

namespace detail {

template <typename Class, typename MemberData>
struct make_const
{
    using type = MemberData;
};

template <typename Class, typename MemberData>
struct make_const<const Class, MemberData>
{
    using type = std::add_const_t<MemberData>;
};

template <typename Class, typename MemberData>
using make_const_t = typename make_const<Class, MemberData>::type;
} // namespace detail

template <typename Parent, typename MemberData>
class field_info
{
public:
    using original_type = MemberData;
    using class_type = Parent;
    // If `Parent` is const we make `type` also const
    using type =
        detail::make_const_t<Parent, std::remove_reference_t<MemberData>>;

public:
    constexpr field_info(type& ref, const char* name) noexcept
        : name_{name}, ref_{ref}
    {
    }

    constexpr const char* name() const noexcept { return name_; }
    constexpr type& get() noexcept { return ref_; }
    constexpr std::add_const_t<type&> get() const noexcept { return ref_; }

private:
    const char* name_;
    type& ref_;
};

template <typename MemberData, typename Parent>
constexpr auto make_field_info(Parent&&, MemberData& field,
                               const char* name) noexcept
{
    // parent_type can be `T` or `const T`
    using parent_type = std::remove_reference_t<Parent>;
    return field_info<parent_type, MemberData>{field, name};
}

template <typename MemberData, typename Parent>
constexpr auto make_field_info(Parent&&, const MemberData& field,
                               const char* name) noexcept
{
    using parent_type = std::remove_reference_t<Parent>;
    return field_info<parent_type, const MemberData>{field, name};
}
} // namespace kl

#define KL_DESCRIBE_BASES(type_, ...)                                          \
    KL_DESCRIBE_BASES_TUPLE(type_, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_DESCRIBE_FIELDS(type_, ...)                                         \
    KL_DESCRIBE_FIELDS_TUPLE(type_, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))

#define KL_DESCRIBE_BASES_TUPLE(type_, bases_)                                 \
    constexpr auto describe_bases(type_*) noexcept                             \
    {                                                                          \
        return ::kl::type_pack<BOOST_PP_REMOVE_PARENS(bases_)>{};              \
    }

#define KL_DESCRIBE_FIELDS_TUPLE(type_, fields_)                               \
    template <typename Self>                                                   \
    constexpr auto describe_fields(type_*, Self&& self) noexcept               \
    {                                                                          \
        return std::make_tuple(                                                \
            KL_DESCRIBE_FIELDS_BUILD_FIELD_INFO_LIST(fields_));                \
    }

#define KL_DESCRIBE_FIELDS_BUILD_FIELD_INFO_LIST(fields_)                      \
    BOOST_PP_ENUM(BOOST_PP_TUPLE_SIZE(fields_),                                \
                  KL_DESCRIBE_FIELDS_FOR_EACH_IN_TUPLE,                        \
                  (fields_, KL_DESCRIBE_FIELDS_ADD_FIELD_INFO))

#define KL_DESCRIBE_FIELDS_ADD_FIELD_INFO(name_)                               \
    ::kl::make_field_info<decltype(self.name_)>(self, self.name_,              \
                                                BOOST_PP_STRINGIZE(name_))

// tuple_macro is ((tuple), macro)
#define KL_DESCRIBE_FIELDS_FOR_EACH_IN_TUPLE(_, index_, tuple_macro_)          \
    BOOST_PP_TUPLE_ELEM(1, tuple_macro_)                                       \
    (BOOST_PP_TUPLE_ELEM(index_, BOOST_PP_TUPLE_ELEM(0, tuple_macro_)))
