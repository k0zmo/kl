#pragma once

#include "kl/type_traits.hpp"
#include "kl/describe_enum.hpp"

#include <boost/optional/optional.hpp>

#include <gsl/string_span>

#include <type_traits>
#include <cstring>

namespace kl {

namespace detail {

KL_VALID_EXPR_HELPER(has_describe_enum, describe_enum(T{}))

// C++14 array lite (TODO: replace with std::array later)
template <typename T, std::size_t N>
struct array14
{
    constexpr auto size() const noexcept { return N; }
    constexpr auto data() const noexcept { return elems_; }
    constexpr T* begin() noexcept { return elems_; }
    constexpr T* end() noexcept { return elems_ + size(); }

    T elems_[N];
};
} // namespace detail

template <typename Enum, bool is_enum = std::is_enum<Enum>::value>
struct is_enum_reflectable : std::false_type {};

// NOTE: We can't instantiate enum_reflector<Enum> with Enum being not a enum
// type. This would cause compilation error on underlying_type alias. That's why
// we check is_defined only if Enum is an actual enum type.
template <typename Enum>
struct is_enum_reflectable<Enum, true>
    : bool_constant<detail::has_describe_enum<Enum>::value>
{
};

template <typename T>
using is_enum_nonreflectable =
    bool_constant<std::is_enum<T>::value && !is_enum_reflectable<T>::value>;

template <typename Enum>
struct enum_reflector
{
    using enum_type = Enum;
    using underlying_type = std::underlying_type_t<enum_type>;

    static_assert(is_enum_reflectable<enum_type>::value,
                  "Enum must be a reflectable enum. "
                  "Define describe_enum(Enum) function");

    static constexpr std::size_t count() noexcept
    {
        return describe_enum(enum_type{}).size();
    }

    // Could be constexpr with std optional and string_view
    static boost::optional<enum_type>
        from_string(gsl::cstring_span<> str) noexcept
    {
        for (const auto& vn : describe_enum(enum_type{}))
        {
            const auto len = std::strlen(vn.name);
            if (len == static_cast<std::size_t>(str.length()) &&
                !std::strncmp(vn.name, str.data(), len))
            {
                return {vn.value};
            }
        }
        return boost::none;
    }

    static constexpr const char* to_string(enum_type value) noexcept
    {
        // NOTE: MSVC2017/2019 goes haywire here when we loop directly over the
        // expression which under the table uses `auto&&` to extend the lifetime
        // of it. Thus we assign the expression to `auto` and only then we loop
        // over it.
        const auto rng = describe_enum(value);
        for (auto& vn : rng)
        {
            if (vn.value == value)
                return vn.name;
        }
        return "(unknown)";
    }

    static kl::range<const enum_type*> values() noexcept
    {
        static constexpr auto value_list = values_impl();
        return {value_list.data(), value_list.data() + value_list.size()};
    }

    static constexpr auto constexpr_values() noexcept { return values_impl(); }

private:
    static constexpr auto values_impl() noexcept
    {
        constexpr auto rng = describe_enum(enum_type{});
        kl::detail::array14<enum_type, rng.size()> values{};
        auto it = rng.begin();
        for (auto& value : values)
        {
            value = it->value;
            ++it;
        }
        return values;
    }
};

template <typename Enum>
constexpr enum_reflector<Enum> reflect() noexcept
{
    static_assert(is_enum_reflectable<Enum>::value,
                  "E must be a reflectable enum - defined using "
                  "KL_DESCRIBE_ENUM macro");
    return {};
}

template <typename Enum, enable_if<is_enum_reflectable<Enum>> = true>
const char* to_string(Enum e) noexcept
{
    return enum_reflector<Enum>::to_string(e);
}

template <typename Enum, enable_if<is_enum_reflectable<Enum>> = true>
boost::optional<Enum> from_string(gsl::cstring_span<> str) noexcept
{
    return enum_reflector<Enum>::from_string(str);
}
} // namespace kl
