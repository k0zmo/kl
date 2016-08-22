#pragma once

/*
 * Requirements: boost 1.57+, C++11 compiler and preprocessor
 * Sample usage:

namespace ns {
namespace detail {
enum class enum_
{
    A, B, C
};
}
}

 * First argument is namespace scope, can be omitted for enums defined in global
   namespace
 * Second argument is unqualified enum type name
 * Third argument is a tuple of enum values. Each value can be optionally a
   tuple (pair) of its real name and name used for to-from string conversions

KL_DEFINE_ENUM_REFLECTOR(ns::detail, enum_,
                         (A, (B, b), C))

 * Remarks: Macro KL_DEFINE_ENUM_REFLECTOR works for unscoped as well as scoped
   enums and requires C++11 (both for preprocessor and unscoped enum handling)

 * KL_DEFINE_ENUM_REFLECTOR(...) defines enum_reflector<Enum> class with
   following members:
    - count(): return number of defined enum values
    - name(): returns string with unqualified name of enum type
    - full_name(): returns string with a name with full namespace scope
    - to_string(): converts given enum value to its string representation
    - from_string(): converts string to enum value
*/

#include <type_traits>
#include <cstring>
#include <boost/optional.hpp>

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>

#include <gsl/string_span.h>

#if defined(_MSC_VER) && BOOST_VERSION == 105700
#undef BOOST_PP_EXPAND_I
#define BOOST_PP_EXPAND_I(...) __VA_ARGS__
#endif

namespace kl {

template <typename Enum>
struct enum_reflector
{
    using enum_type = Enum;
    using underlying_type = std::underlying_type_t<enum_type>;

    static constexpr std::size_t count() { return 0; }
    static constexpr const char* name() { return ""; }
    static constexpr const char* full_name() { return ""; }
    static constexpr const bool is_defined = false;

    static boost::optional<enum_type> from_string(gsl::cstring_span<> str)
    {
        return boost::none;
    }
    static const char* to_string(enum_type value) { return nullptr; }
};

template <typename Enum, typename = void>
struct is_enum_reflectable : std::false_type {};

// NOTE: We can't instantiate enum_reflector<Enum> with Enum being not a enum
// type. This would cause compilation error on underlying_type alias. That's why
// we check is_defined only if Enum is an actual enum type.
template <typename Enum>
struct is_enum_reflectable<Enum, std::enable_if_t<std::is_enum<Enum>::value>>
    : std::integral_constant<bool, enum_reflector<Enum>::is_defined>
{
};

template <typename T>
using is_enum_nonreflectable =
    std::integral_constant<bool, std::is_enum<T>::value &&
                                     !is_enum_reflectable<T>::value>;
} // namespace kl

#define KL_DEFINE_ENUM_REFLECTOR(...) KL_ENUM_REFLECTOR_IMPL(__VA_ARGS__)
#if defined(_MSC_VER) && !defined(__INTELLISENSE__)
#define KL_ENUM_REFLECTOR_IMPL(...)                                            \
    BOOST_PP_CAT(                                                              \
        BOOST_PP_OVERLOAD(KL_ENUM_REFLECTOR_IMPL_, __VA_ARGS__)(__VA_ARGS__),  \
        BOOST_PP_EMPTY())
#else
#define KL_ENUM_REFLECTOR_IMPL(...)                                            \
    BOOST_PP_OVERLOAD(KL_ENUM_REFLECTOR_IMPL_, __VA_ARGS__)(__VA_ARGS__)
#endif

// for enums in global namespace
#define KL_ENUM_REFLECTOR_IMPL_2(name_, values_)                               \
    KL_ENUM_REFLECTOR_DEFINITION(_, name_, values_)

// for enums enclosed in some namespace(s)
#define KL_ENUM_REFLECTOR_IMPL_3(ns_, name_, values_)                          \
    KL_ENUM_REFLECTOR_DEFINITION(KL_ENUM_REFLECTOR_ARG_TO_TUPLE(ns_), name_,   \
                                 values_)

#define KL_ENUM_REFLECTOR_DEFINITION(ns_, name_, values_)                      \
    KL_ENUM_REFLECTOR_DEFINITION_IMPL(                                         \
        KL_ENUM_REFLECTOR_FULL_NAME(ns_, name_),                               \
        KL_ENUM_REFLECTOR_FULL_NAME_STRING(ns_, name_), name_,                 \
        (KL_ENUM_REFLECTOR_ARGS_TO_TUPLES(values_)))

#define KL_ENUM_REFLECTOR_DEFINITION_IMPL(full_name_, full_name_string_,       \
                                          name_, values_)                      \
    namespace kl {                                                             \
    template <>                                                                \
    struct enum_reflector<full_name_>                                          \
    {                                                                          \
        using enum_type = full_name_;                                          \
        using underlying_type = std::underlying_type_t<enum_type>;             \
                                                                               \
        static constexpr std::size_t count()                                   \
        {                                                                      \
            return BOOST_PP_TUPLE_SIZE(values_);                               \
        }                                                                      \
        static constexpr const char* name()                                    \
        {                                                                      \
            return BOOST_PP_STRINGIZE(name_);                                  \
        }                                                                      \
        static constexpr const char* full_name() { return full_name_string_; } \
        static constexpr const bool is_defined = true;                         \
                                                                               \
        KL_ENUM_REFLECTOR_DEFINITION_FROM_STRING(full_name_, values_)          \
        KL_ENUM_REFLECTOR_DEFINITION_TO_STRING(full_name_, values_)            \
    };                                                                         \
    }

#define KL_ENUM_REFLECTOR_GET_ENUM_VALUE(arg_) BOOST_PP_TUPLE_ELEM(0, arg_)

#define KL_ENUM_REFLECTOR_GET_ENUM_VALUE_STRING_IMPL(arg_)                     \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(arg_)),                       \
                BOOST_PP_TUPLE_ELEM(1, arg_), BOOST_PP_TUPLE_ELEM(0, arg_))

#define KL_ENUM_REFLECTOR_GET_ENUM_VALUE_STRING(arg_)                          \
    BOOST_PP_STRINGIZE(KL_ENUM_REFLECTOR_GET_ENUM_VALUE_STRING_IMPL(arg_))

#define KL_ENUM_REFLECTOR_VALUE_FROM_STRING(full_name_, value_)                \
    if (str == KL_ENUM_REFLECTOR_GET_ENUM_VALUE_STRING(value_))                \
        return full_name_::KL_ENUM_REFLECTOR_GET_ENUM_VALUE(value_);

#define KL_ENUM_REFLECTOR_DEFINITION_FROM_STRING(full_name_, values_)          \
    static boost::optional<enum_type> from_string(gsl::cstring_span<> str)     \
    {                                                                          \
        BOOST_PP_REPEAT(                                                       \
            BOOST_PP_TUPLE_SIZE(values_),                                      \
            KL_ENUM_REFLECTOR_FOR_EACH_IN_TUPLE2,                              \
            (full_name_, values_, KL_ENUM_REFLECTOR_VALUE_FROM_STRING))        \
        return boost::none;                                                    \
    }

#define KL_ENUM_REFLECTOR_VALUE_TO_STRING(full_name_, value_)                  \
    case full_name_::KL_ENUM_REFLECTOR_GET_ENUM_VALUE(value_):                 \
        return KL_ENUM_REFLECTOR_GET_ENUM_VALUE_STRING(value_);

#define KL_ENUM_REFLECTOR_DEFINITION_TO_STRING(full_name_, values_)            \
    static const char* to_string(enum_type value)                              \
    {                                                                          \
        switch (value)                                                         \
        {                                                                      \
            BOOST_PP_REPEAT(                                                   \
                BOOST_PP_TUPLE_SIZE(values_),                                  \
                KL_ENUM_REFLECTOR_FOR_EACH_IN_TUPLE2,                          \
                (full_name_, values_, KL_ENUM_REFLECTOR_VALUE_TO_STRING))      \
        }                                                                      \
        return "(unknown)";                                                    \
    }

// makes sure arg is a tuple (works for tuples and single arg)
#define KL_ENUM_REFLECTOR_ARG_TO_TUPLE(arg_) (BOOST_PP_REMOVE_PARENS(arg_))

// (x) -> (x, x)
// (x, y) -> (x, y)   :nop
// (x, y, z) -> (x, y, z) :nop
#define KL_ENUM_REFLECTOR_ARG_TO_TUPLES_TRANSFORM(arg_)                        \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(arg_)), arg_,                 \
                BOOST_PP_TUPLE_PUSH_BACK(arg_, BOOST_PP_TUPLE_ELEM(0, arg_)))

#define KL_ENUM_REFLECTOR_ARG_TO_TUPLES(arg_)                                  \
    KL_ENUM_REFLECTOR_ARG_TO_TUPLES_TRANSFORM(                                 \
        KL_ENUM_REFLECTOR_ARG_TO_TUPLE(arg_))

#define KL_ENUM_REFLECTOR_ARGS_TO_TUPLES_IMPL(_, index_, args_)                \
    KL_ENUM_REFLECTOR_ARG_TO_TUPLES(BOOST_PP_TUPLE_ELEM(index_, args_))

#define KL_ENUM_REFLECTOR_ARGS_TO_TUPLES(args_)                                \
    BOOST_PP_ENUM(BOOST_PP_TUPLE_SIZE(args_),                                  \
                  KL_ENUM_REFLECTOR_ARGS_TO_TUPLES_IMPL, args_)

// tuple_macro_ is ((tuple), macro)
#define KL_ENUM_REFLECTOR_FOR_EACH_IN_TUPLE(_, index_, tuple_macro_)           \
    BOOST_PP_TUPLE_ELEM(1, tuple_macro_)                                       \
    (BOOST_PP_TUPLE_ELEM(index_, BOOST_PP_TUPLE_ELEM(0, tuple_macro_)))

// tuple_macro_ is (arg, (tuple), macro)
#define KL_ENUM_REFLECTOR_FOR_EACH_IN_TUPLE2(_, index_, tuple_macro_)          \
    BOOST_PP_TUPLE_ELEM(2, tuple_macro_)                                       \
    (BOOST_PP_TUPLE_ELEM(0, tuple_macro_),                                     \
     BOOST_PP_TUPLE_ELEM(index_, BOOST_PP_TUPLE_ELEM(1, tuple_macro_)))

#define KL_ENUM_REFLECTOR_NAMESPACE_SCOPE(ns_) ns_::

#define KL_ENUM_REFLECTOR_FULL_NAME(ns_, name_)                                \
    BOOST_PP_EXPR_IF(                                                          \
        BOOST_PP_IS_BEGIN_PARENS(ns_),                                         \
        BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(ns_),                              \
                        KL_ENUM_REFLECTOR_FOR_EACH_IN_TUPLE,                   \
                        (ns_, KL_ENUM_REFLECTOR_NAMESPACE_SCOPE)))             \
    name_

#define KL_ENUM_REFLECTOR_NAMESPACE_SCOPE_STRING(ns_) BOOST_PP_STRINGIZE(ns_::)

#define KL_ENUM_REFLECTOR_FULL_NAME_STRING(ns_, name_)                         \
    BOOST_PP_EXPR_IF(                                                          \
        BOOST_PP_IS_BEGIN_PARENS(ns_),                                         \
        BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(ns_),                              \
                        KL_ENUM_REFLECTOR_FOR_EACH_IN_TUPLE,                   \
                        (ns_, KL_ENUM_REFLECTOR_NAMESPACE_SCOPE_STRING)))      \
    BOOST_PP_STRINGIZE(name_)
