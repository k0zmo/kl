#pragma once

#include "kl/binary_rw.hpp"
#include "kl/meta.hpp"

#include <boost/variant.hpp>

namespace kl {
namespace detail {

template <typename Variant>
void visit_by_index(kl::binary_reader& r, kl::type_pack<>, Variant&,
                    std::uint8_t)
{
    r.notify_error();
}

template <typename Head, typename... Tail, typename Variant>
void visit_by_index(kl::binary_reader& r, kl::type_pack<Head, Tail...>,
                    Variant& variant, std::uint8_t which)
{
    if (which == 0)
    {
        Head value = r.read<Head>();
        if (!r.err())
            variant = std::move(value);
    }
    else
    {
        visit_by_index(r, kl::type_pack<Tail...>{}, variant, which - 1);
    }
}

template <typename T>
struct not_boost_variant_void
    : kl::bool_constant<!std::is_same<T, boost::detail::variant::void_>::value>
{
};

template <typename... Args>
void decode_variant(kl::binary_reader& r, boost::variant<Args...>& var,
                    std::uint8_t which)
{
    // boost::variant<int, bool> is really
    // boost::variant<int, bool, boost::detail::variant::void_, ...>
    // and we'd like to get type list without this noise
    using args_list = typename filter<not_boost_variant_void, Args...>::type;
    visit_by_index(r, args_list{}, var, which);
}
} // namespace detail

template <typename... Args>
kl::binary_reader& operator>>(kl::binary_reader& r,
                              boost::variant<Args...>& var)
{
    const auto which = r.read<std::uint8_t>();
    if (!r.err())
        detail::decode_variant(r, var, which);
    return r;
}
} // namespace kl
