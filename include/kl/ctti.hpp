#pragma once

#include "kl/tuple.hpp"
#include "kl/type_traits.hpp"
#include "kl/describe_record.hpp"

#include <boost/type_index.hpp>

#include <type_traits>
#include <string>

namespace kl {
namespace detail {

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

template <typename T, typename U>
struct mirror_referenceness
{
    using type = U;
};

template <typename T, typename U>
struct mirror_referenceness<T&, U>
{
    using type = U&;
};

template <typename T, typename U>
struct mirror_referenceness<const T&, U>
{
    using type = const U&;
};

template <typename T, typename U>
struct mirror_referenceness<T&&, U>
{
    using type = U&&;
};

template <typename T, typename U>
struct mirror_referenceness<const T&&, U>
{
    using type = const U&&;
};
} // namespace detail

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
        static_assert(
            std::is_base_of<Head, std::remove_reference_t<Reflectable>>::value,
            "Head is not a base of Reflectable");
        using head_type =
            typename detail::mirror_referenceness<Reflectable, Head>::type;
        reflect(static_cast<head_type>(r), v);
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
