#pragma once

#include "kl/type_traits.hpp"
#include "kl/reflect_enum.hpp"
#include "kl/range.hpp"

#include <string_view>
#include <type_traits>
#include <array>
#include <optional>

namespace kl {

namespace detail {

KL_VALID_EXPR_HELPER(has_reflect_enum, reflect_enum(::kl::enum_<T>))
} // namespace detail

template <typename Enum, bool is_enum = std::is_enum_v<Enum>>
struct is_enum_reflectable : std::false_type {};

// NOTE: We can't instantiate enum_reflector<Enum> with Enum being not a enum
// type. This would cause compilation error on underlying_type alias. That's why
// we check is_defined only if Enum is an actual enum type.
template <typename Enum>
struct is_enum_reflectable<Enum, true>
    : std::bool_constant<detail::has_reflect_enum_v<Enum>>
{
};

template <typename T>
inline constexpr bool is_enum_reflectable_v = is_enum_reflectable<T>::value;

template <typename Enum>
struct enum_reflector
{
    using enum_type = Enum;
    using underlying_type = std::underlying_type_t<enum_type>;

    static_assert(is_enum_reflectable_v<enum_type>,
                  "Enum must be a reflectable enum. "
                  "Define reflect_enum(kl::enum_class<Enum>) function");

    static constexpr std::size_t count() noexcept
    {
        return reflect_enum(enum_<enum_type>).size();
    }

    static constexpr std::optional<enum_type>
        from_string(std::string_view str) noexcept
    {
        // NOTE: MSVC2017/2019 goes haywire here when we loop directly over the
        // expression which under the table uses `auto&&` to extend the lifetime
        // of it. Thus we assign the expression to `auto` and only then we loop
        // over it.
        const auto rng = reflect_enum(enum_<enum_type>);
        for (const auto& vn : rng)
        {
            if (str == vn.name)
                return {vn.value};
        }
        return std::nullopt;
    }

    static constexpr const char* to_string(enum_type value) noexcept
    {
        const auto rng = reflect_enum(enum_<enum_type>);

        // Help GCC and MSVC do a lookup (Clang is smart enough) in common
        // case when enum values starts from 0 and does not have any wholes
        const auto num_value = static_cast<std::size_t>(value);
        if (num_value < rng.size())
        {
            const auto it = rng.begin() + num_value;
            if (it->value == value)
                return it->name;
        }

        for (const auto& vn : rng)
        {
            if (vn.value == value)
                return vn.name;
        }
        return "(unknown)";
    }

    static auto values() noexcept
    {
        static constexpr auto value_list = values_impl();
        static constexpr auto first = value_list.data();
        static constexpr auto last = first + value_list.size();
        return kl::range{first, last};
    }

    static constexpr auto constexpr_values() noexcept { return values_impl(); }

private:
    static constexpr auto values_impl() noexcept
    {
        constexpr auto rng = reflect_enum(enum_<enum_type>);
        std::array<enum_type, rng.size()> values{};
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
    static_assert(is_enum_reflectable_v<Enum>,
                  "E must be a reflectable enum - defined using "
                  "KL_REFLECT_ENUM macro");
    return {};
}

template <typename Enum, enable_if<is_enum_reflectable<Enum>> = true>
const char* to_string(Enum e) noexcept
{
    return enum_reflector<Enum>::to_string(e);
}

template <typename Enum, enable_if<is_enum_reflectable<Enum>> = true>
std::optional<Enum> from_string(std::string_view str) noexcept
{
    return enum_reflector<Enum>::from_string(str);
}
} // namespace kl
