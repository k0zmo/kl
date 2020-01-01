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
inline constexpr auto null = static_cast<remove_cvref_t<T>*>(nullptr);

KL_VALID_EXPR_HELPER(has_describe_bases, describe_bases(null<T>))
KL_VALID_EXPR_HELPER(has_describe_fields,
                     describe_fields(null<T>, std::declval<T&>()))

template <typename T, bool is_describe_bases_defined = has_describe_bases_v<T>>
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
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

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
    template <typename T>
    static constexpr bool is_reflectable = kl::is_reflectable_v<T>;

    template <typename Reflected>
    using base_types = typename detail::base_types<Reflected>::type;

    template <typename Reflected>
    static std::string name() noexcept
    {
        return boost::typeindex::type_id<Reflected>().pretty_name();
    }

    template <typename Reflected, typename Visitor>
    static constexpr void reflect(Reflected&& r, Visitor&& v)
    {
        static_assert(
            detail::has_describe_fields_v<Reflected>,
            "Can't reflect this type. Define describe_fields function");

        reflect_bases(r, v, base_types<Reflected>{});
        tuple::for_each_fn::call(describe_fields(std::forward<Reflected>(r)),
                                 std::forward<Visitor>(v));
    }

    template <typename Reflected>
    static constexpr std::size_t num_fields() noexcept
    {
        static_assert(
            detail::has_describe_fields_v<Reflected>,
            "Can't reflect this type. Define describe_fields function");

        using field_desc =
            decltype(describe_fields(std::declval<Reflected&>()));
        return std::tuple_size_v<field_desc>;
    }

    template <typename Reflected>
    static constexpr std::size_t total_num_fields() noexcept
    {
        return num_fields<Reflected>() +
               base_num_fields(base_types<Reflected>{});
    }

private:
    template <typename Base, typename Reflected, typename Visitor>
    static constexpr void reflect_base(Reflected&& r, Visitor&& v)
    {
        static_assert(
            std::is_base_of<Base, std::remove_reference_t<Reflected>>::value,
            "Base is not a base of Reflected");
        using base_type =
            typename detail::mirror_referenceness<Reflected, Base>::type;
        reflect(static_cast<base_type>(r), std::forward<Visitor>(v));
    }

    template <typename Reflected, typename Visitor, typename... Bases>
    static constexpr void reflect_bases(Reflected&& r, Visitor&& v,
                                        type_pack<Bases...>)
    {
        (reflect_base<Bases>(r, v), ...);
    }

    template <typename... Bases>
    static constexpr std::size_t base_num_fields(type_pack<Bases...>) noexcept
    {
        return (0 + ... + total_num_fields<Bases>());
    }
};
} // namespace kl
