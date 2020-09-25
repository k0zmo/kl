#pragma once

#include "kl/type_traits.hpp"
#include "kl/reflect_struct.hpp"

#include <boost/type_index.hpp>

#include <type_traits>
#include <string>

namespace kl {
namespace detail {

KL_VALID_EXPR_HELPER(has_reflect_struct,
                     reflect_struct(0, std::declval<T&>(), record<T>))

} // namespace detail

template <typename T>
using is_reflectable = detail::has_reflect_struct<T>;

template <typename T>
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

struct ctti
{
    template <typename T>
    static constexpr bool is_reflectable = kl::is_reflectable_v<T>;

    template <typename Reflected>
    static std::string name()
    {
        return boost::typeindex::type_id<Reflected>().pretty_name();
    }

    template <typename Reflected, typename Visitor>
    static constexpr void reflect(Reflected&& r, Visitor&& v)
    {
        using R = remove_cvref_t<Reflected>;

        static_assert(
            detail::has_reflect_struct_v<R>,
            "Can't reflect this type. Define reflect_struct function");
        reflect_struct(std::forward<Visitor>(v), std::forward<Reflected>(r),
                       record<R>);
    }

    template <typename Reflected>
    static constexpr std::size_t num_fields() noexcept
    {
        using R = remove_cvref_t<Reflected>;

        static_assert(
            detail::has_reflect_struct_v<R>,
            "Can't reflect this type. Define reflect_struct function");
        return reflect_num_fields(record<R>);
    }
};
} // namespace kl
