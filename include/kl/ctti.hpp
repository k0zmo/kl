#pragma once

#include "kl/tuple.hpp"
#include "kl/type_traits.hpp"

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/type_index.hpp>

#include <type_traits>
#include <string>

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

template <typename T>
/*inline*/ constexpr auto null = static_cast<remove_cvref_t<T>*>(nullptr);

KL_VALID_EXPR_HELPER(has_describe_bases, describe_bases(null<T>))
KL_VALID_EXPR_HELPER(has_describe_fields,
                     describe_fields(null<T>, std::declval<T&>()))

template <typename T,
          bool is_describe_bases_defined = has_describe_bases<T>::value>
struct base_types
{
    using type = type_pack<>;
};

template <typename T>
struct base_types<T, true>
{
    using type = decltype(describe_bases(null<T>));
};
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

template <typename T>
using is_reflectable = detail::has_describe_fields<T>;

template <typename T>
constexpr auto describe_fields(T&& obj) noexcept
{
    return describe_fields(detail::null<T>, std::forward<T>(obj));
}

template <typename T>
constexpr auto describe_bases() noexcept
{
    return describe_bases(detail::null<T>);
}

struct ctti
{
    template <typename Reflectable>
    using base_types = typename detail::base_types<Reflectable>::type;

    template <typename Reflectable>
    static constexpr bool is_reflectable =
        kl::is_reflectable<Reflectable>::value;

    template <typename Reflectable>
    static std::string name() noexcept
    {
        return boost::typeindex::type_id<Reflectable>().pretty_name();
    }

    template <typename Reflectable, typename Visitor>
    static constexpr void reflect(Reflectable&& r, Visitor&& v)
    {
        static_assert(
            detail::has_describe_fields<Reflectable>::value,
            "Can't reflect this type. Define describe_fields function");

        reflect_bases(std::forward<Reflectable>(r), v,
                      base_types<Reflectable>{});
        tuple::for_each_fn::call(describe_fields(std::forward<Reflectable>(r)),
                                 v);
    }

    template <typename Reflectable>
    static constexpr std::size_t num_fields() noexcept
    {
        static_assert(
            detail::has_describe_fields<Reflectable>::value,
            "Can't reflect this type. Define describe_fields function");

        using field_desc =
            decltype(describe_fields(std::declval<Reflectable&>()));
        return std::tuple_size<field_desc>::value;
    }

    template <typename Reflectable>
    static constexpr std::size_t total_num_fields() noexcept
    {
        return num_fields<Reflectable>() +
               base_num_fields(base_types<Reflectable>{});
    }

private:
    template <typename Reflectable, typename Visitor>
    static constexpr void reflect_bases(Reflectable&&, Visitor&&, type_pack<>)
    {
    }

    template <typename Reflectable, typename Visitor, typename Head,
              typename... Tail>
    static constexpr void reflect_bases(Reflectable&& r, Visitor&& v,
                                        type_pack<Head, Tail...>)
    {
        reflect(std::forward<Head>(r), v);
        reflect_bases(std::forward<Reflectable>(r), v, type_pack<Tail...>{});
    }

    static constexpr std::size_t base_num_fields(type_pack<>) noexcept
    {
        return 0;
    }

    template <typename Head, typename... Tail>
    static constexpr std::size_t
        base_num_fields(type_pack<Head, Tail...>) noexcept
    {
        return total_num_fields<Head>() + base_num_fields(type_pack<Tail...>{});
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
