#pragma once

#include "kl/type_traits.hpp"
#include "kl/reflect_struct.hpp"

#include <boost/core/demangle.hpp>

#include <cstddef>
#include <string>
#include <typeinfo>
#include <utility>

namespace kl::ctti {
namespace detail {

KL_VALID_EXPR_HELPER(has_reflect_struct,
                     reflect_struct(0, std::declval<T&>(), record<T>))

} // namespace detail

template <typename T>
using is_reflectable = detail::has_reflect_struct<T>;

template <typename T>
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

template <typename Reflected>
std::string name()
{
    using drop_cv_ref = remove_cvref_t<Reflected>;
    const std::type_info& ti = typeid(drop_cv_ref);
    boost::core::scoped_demangled_name demangled{ti.name()};
    return std::string{demangled.get()};
}

template <typename Reflected, typename Visitor>
constexpr void reflect(Reflected&& r, Visitor&& v)
{
    using R = remove_cvref_t<Reflected>;

    static_assert(detail::has_reflect_struct_v<R>,
                  "Can't reflect this type. Define reflect_struct function");
    reflect_struct(std::forward<Visitor>(v), std::forward<Reflected>(r), record<R>);
}

template <typename Reflected>
constexpr std::size_t num_fields() noexcept
{
    using R = remove_cvref_t<Reflected>;

    static_assert(detail::has_reflect_struct_v<R>,
                  "Can't reflect this type. Define reflect_struct function");
    return reflect_num_fields(record<R>);
}

template <typename... Attributes, typename Reflectable>
constexpr bool has_any_attribute(const Reflectable& refl)
{
    bool result = false;
    ctti::reflect(refl, [&result](auto field) {
        using field_type = decltype(field);
        if constexpr((field_type::template has<Attributes>() || ...))
        {
            result = true;
        }
    });
    return result;
}

} // namespace kl::ctti
