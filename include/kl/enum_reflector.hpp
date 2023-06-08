#pragma once

#include "kl/type_traits.hpp"
#include "kl/reflect_enum.hpp"
#include "kl/range.hpp"
#include "kl/utility.hpp"

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <array>
#include <optional>

namespace kl {

template <typename T>
concept reflectable_enum = std::is_enum_v<T> &&
    requires(T) {
        { reflect_enum(::kl::enum_<T>) };
    };

template <typename T>
using is_enum_reflectable = std::bool_constant<reflectable_enum<T>>;

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
        for (const auto& vn : reflect_enum(enum_<enum_type>))
        {
            if (str == vn.name)
                return {vn.value};
        }
        return std::nullopt;
    }

    static constexpr const char* to_string(
        enum_type value,
        const char* def = reflect_enum_unknown_name(enum_<enum_type>)) noexcept
    {
        constexpr auto rng = reflect_enum(enum_<enum_type>);

        // For a usual case when enum type starts from 0 and does not have any holes
        if constexpr (is_ordinary_enum())
        {
            const auto num_value = static_cast<std::size_t>(value);
            return num_value < rng.size() ? (rng.begin() + num_value)->name
                                          : def;
        }
        else
        {
            for (const auto& vn : rng)
            {
                if (vn.value == value)
                    return vn.name;
            }
            return def;
        }
    }

    static auto values() noexcept
    {
        static constexpr auto value_list = values_impl();
        static constexpr auto first = value_list.data();
        static constexpr auto last = first + value_list.size();
        return kl::range{first, last};
    }

    static constexpr auto constexpr_values() noexcept { return values_impl(); }

    static constexpr bool is_ordinary_enum() noexcept
    {
        constexpr auto rng = reflect_enum(enum_<enum_type>);

        // Check if first enum value is 0 and the last enum value is equal to number of enum values minus 0
        constexpr auto first_value = rng.begin()->value;
        constexpr auto last_value = (rng.begin() + rng.size() - 1)->value;
        if (first_value != static_cast<Enum>(0) ||
            last_value != static_cast<Enum>(rng.size() - 1))
        {
            return false;
        }
        // Lastly, check if all the values are sorted - otherwise enum with values
        // (0, 3, 2) would be considered 'ordinary' which is far from the true
        auto cur = rng.begin(), next = cur;
        while (++next != rng.end())
        {
            if (cur->value >= next->value)
                return false;
            cur = next;
        }
        return true;
    }

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

template <reflectable_enum E>
const char* to_string(E e) noexcept
{
    return enum_reflector<E>::to_string(e);
}

template <reflectable_enum E>
std::optional<E> from_string(std::string_view str) noexcept
{
    return enum_reflector<E>::from_string(str);
}
} // namespace kl
