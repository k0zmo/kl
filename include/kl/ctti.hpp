#pragma once

#include "kl/type_traits.hpp"
#include "kl/reflect_struct.hpp"

#include <boost/core/demangle.hpp>

#include <cstddef>
#include <typeinfo>
#include <string>

namespace kl {

template <typename T>
concept reflectable =
    requires(T&& a) {
        { reflect_struct(0, a, kl::record<std::remove_cvref_t<T>>) };
    };

template <typename T>
using is_reflectable = std::bool_constant<reflectable<T>>;

template <typename T>
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

struct ctti
{
    template <typename R>
    static std::string name()
    {
        using drop_cv_ref = std::remove_cvref_t<R>;
        const std::type_info& ti = typeid(drop_cv_ref);
        boost::core::scoped_demangled_name demangled{ti.name()};
        return std::string{demangled.get()};
    }

    template <reflectable R, typename Visitor>
    static constexpr void reflect(R&& r, Visitor&& v)
    {
        using PlainR = std::remove_cvref_t<R>;
        reflect_struct(std::forward<Visitor>(v), std::forward<R>(r),
                       record<PlainR>);
    }

    template <reflectable R>
    static constexpr std::size_t num_fields() noexcept
    {
        using PlainR = std::remove_cvref_t<R>;
        return reflect_num_fields(record<PlainR>);
    }
};
} // namespace kl
