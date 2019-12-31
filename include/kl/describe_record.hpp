#pragma once

#include "kl/utility.hpp"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>

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
KL_DESCRIBE_FIELDS(A, (i, b, d))

struct B : A
{
    std::string str;
};
KL_DESCRIBE_BASES(B, (A))
KL_DESCRIBE_FIELDS(B, (str))
}

 KL_DESCRIBE_FIELDS
 ==================

 * First argument is unqualified type name
 * Second argument is a tuple of all fields that need to be visible by kl::ctti
 * Macro should be placed in the same namespace as the type
 * Above definitions are expanded to:

     template <typename Self>
     constexpr auto describe_fields(A*, Self&& self) noexcept
     {
         using builder = ::kl::field_info_builder<Self>;
         return std::make_tuple(builder::add(self.i, "i"),
                                builder::add(self.b, "b"),
                                builder::add(self.d, "d"));
     }

     template <typename Self>
     constexpr auto describe_fields(B*, Self&& self) noexcept
     {
         using builder = ::kl::field_info_builder<Self>;
         return std::make_tuple(builder::add(self.str, "str"));
     }

 KL_DESCRIBE_BASES
 =================

 * First argument is unqualified type name
 * Second argument is a tuple of all _direct_ base classes of the type
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
    using type = detail::make_const_t<Parent, MemberData>;

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

template <typename Parent>
struct field_info_builder
{
    using parent_type = std::remove_reference_t<Parent>;

    template <typename MemberData>
    static constexpr auto add(MemberData&& field, const char* name) noexcept
    {
        return field_info<parent_type, std::remove_reference_t<MemberData>>{
            field, name};
    }
};
} // namespace kl

#define KL_DESCRIBE_BASES(type_, bases_)                                       \
    constexpr auto describe_bases(type_*) noexcept                             \
    {                                                                          \
        return ::kl::type_pack<BOOST_PP_REMOVE_PARENS(bases_)>{};              \
    }

#define KL_DESCRIBE_FIELDS(type_, fields_)                                     \
    template <typename Self>                                                   \
    constexpr auto describe_fields(type_*, Self&& self) noexcept               \
    {                                                                          \
        using builder = ::kl::field_info_builder<Self>;                        \
        return std::make_tuple(                                                \
            KL_DESCRIBE_FIELDS_BUILD_FIELD_INFO_LIST(fields_));                \
    }

#define KL_DESCRIBE_FIELDS_BUILD_FIELD_INFO_LIST(fields_)                      \
    BOOST_PP_ENUM(BOOST_PP_TUPLE_SIZE(fields_),                                \
                  KL_DESCRIBE_FIELDS_FOR_EACH_IN_TUPLE,                        \
                  (fields_, KL_DESCRIBE_FIELDS_ADD_FIELD_INFO))

#define KL_DESCRIBE_FIELDS_ADD_FIELD_INFO(name_)                               \
    builder::add(self.name_, BOOST_PP_STRINGIZE(name_))

// tuple_macro is ((tuple), macro)
#define KL_DESCRIBE_FIELDS_FOR_EACH_IN_TUPLE(_, index_, tuple_macro_)          \
    BOOST_PP_TUPLE_ELEM(1, tuple_macro_)                                       \
    (BOOST_PP_TUPLE_ELEM(index_, BOOST_PP_TUPLE_ELEM(0, tuple_macro_)))
