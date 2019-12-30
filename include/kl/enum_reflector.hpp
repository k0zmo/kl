#pragma once

/*
 * Requirements: boost 1.57+, C++14 compiler and preprocessor
 * Sample usage:

namespace ns {
namespace detail {
enum class enum_
{
    A, B, C
};
KL_DESCRIBE_ENUM(enum_, (A, (B, b), C))
}
}

 * First argument is unqualified enum type name
 * Second argument is a tuple of enum values. Each value can be optionally a
   tuple (pair) of its real name and name used for to-from string conversions
 * Macro should be placed inside the same namespace as the enum type

 * Remarks: Macro KL_DESCRIBE_ENUM works for unscoped as well as scoped
   enums and requires C++17 (both for preprocessor and unscoped enum handling)
*/

#include "kl/range.hpp"
#include "kl/type_traits.hpp"

#include <boost/optional/optional.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <gsl/string_span>

#include <type_traits>
#include <cstring>

namespace kl {

template <typename Enum>
struct enum_value_name
{
    Enum value;
    const char* name;
};

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
                !std::strcmp(vn.name, str.data()))
            {
                return {vn.value};
            }
        }
        return boost::none;
    }

    static constexpr const char* to_string(enum_type value) noexcept
    {
        // NOTE: for-range loop with describe_enum(...) causes MSVC2017/2019 to
        // go haywire. `auto&&` is the culprit here though `const auto` is ok.
        const auto rng = describe_enum(value);
        for (auto it = rng.begin(); it != rng.end(); ++it)
        {
            if (it->value == value)
                return it->name;
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

#define KL_DESCRIBE_ENUM(name_, values_)                                       \
    KL_DESCRIBE_ENUM_IMPL(name_, values_, __COUNTER__)

#define KL_DESCRIBE_ENUM_IMPL(name_, values_, counter_)                        \
    static constexpr ::kl::enum_value_name<name_> KL_DESCRIBE_ENUM_VAR_NAME(   \
        counter_)[] = {                                                        \
        KL_DESCRIBE_ENUM_VALUE_NAME_PAIRS(                                     \
            name_, (KL_DESCRIBE_ENUM_ARGS_TO_TUPLES(values_)))};               \
    constexpr auto describe_enum(name_) noexcept                               \
    {                                                                          \
        return ::kl::make_range(KL_DESCRIBE_ENUM_VAR_NAME(counter_));          \
    }

#define KL_DESCRIBE_ENUM_VAR_NAME(counter_)                                    \
    BOOST_PP_CAT(kl_enum_description, counter_)

// Assumes value_ is a tuple: (x, y) or (x, x)
#define KL_DESCRIBE_ENUM_VALUE_NAME_PAIR(name_, value_)                        \
    {                                                                          \
        name_::KL_DESCRIBE_ENUM_GET_ENUM_VALUE(value_),                        \
        KL_DESCRIBE_ENUM_GET_ENUM_STRING(value_),                              \
    },

#define KL_DESCRIBE_ENUM_VALUE_NAME_PAIRS(name_, values_)                      \
    BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(values_),                              \
                    KL_DESCRIBE_ENUM_FOR_EACH_IN_TUPLE,                        \
                    (name_, values_, KL_DESCRIBE_ENUM_VALUE_NAME_PAIR))

#define KL_DESCRIBE_ENUM_GET_ENUM_VALUE(arg_) BOOST_PP_TUPLE_ELEM(0, arg_)

#define KL_DESCRIBE_ENUM_GET_ENUM_STRING_IMPL(arg_)                            \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(arg_)),                       \
                BOOST_PP_TUPLE_ELEM(1, arg_), BOOST_PP_TUPLE_ELEM(0, arg_))

#define KL_DESCRIBE_ENUM_GET_ENUM_STRING(arg_)                                 \
    BOOST_PP_STRINGIZE(KL_DESCRIBE_ENUM_GET_ENUM_STRING_IMPL(arg_))

// makes sure arg is a tuple (works for tuples and single arg)
#define KL_DESCRIBE_ENUM_ARG_TO_TUPLE(arg_) (BOOST_PP_REMOVE_PARENS(arg_))

// (x) -> (x, x)
// (x, y) -> (x, y)   :nop
// (x, y, z) -> (x, y, z) :nop
#define KL_DESCRIBE_ENUM_ARG_TO_TUPLES_TRANSFORM(arg_)                         \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(arg_)), arg_,                 \
                BOOST_PP_TUPLE_PUSH_BACK(arg_, BOOST_PP_TUPLE_ELEM(0, arg_)))

#define KL_DESCRIBE_ENUM_ARG_TO_TUPLES(arg_)                                   \
    KL_DESCRIBE_ENUM_ARG_TO_TUPLES_TRANSFORM(                                  \
        KL_DESCRIBE_ENUM_ARG_TO_TUPLE(arg_))

#define KL_DESCRIBE_ENUM_ARGS_TO_TUPLES_IMPL(_, index_, args_)                 \
    KL_DESCRIBE_ENUM_ARG_TO_TUPLES(BOOST_PP_TUPLE_ELEM(index_, args_))

#define KL_DESCRIBE_ENUM_ARGS_TO_TUPLES(args_)                                 \
    BOOST_PP_ENUM(BOOST_PP_TUPLE_SIZE(args_),                                  \
                  KL_DESCRIBE_ENUM_ARGS_TO_TUPLES_IMPL, args_)

// tuple_macro_ is (arg, (tuple), macro)
#define KL_DESCRIBE_ENUM_FOR_EACH_IN_TUPLE(_, index_, tuple_macro_)            \
    BOOST_PP_TUPLE_ELEM(2, tuple_macro_)                                       \
    (BOOST_PP_TUPLE_ELEM(0, tuple_macro_),                                     \
     BOOST_PP_TUPLE_ELEM(index_, BOOST_PP_TUPLE_ELEM(1, tuple_macro_)))
