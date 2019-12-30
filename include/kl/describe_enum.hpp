#pragma once

#include "kl/range.hpp"

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

namespace kl {

template <typename Enum>
struct enum_value_name
{
    Enum value;
    const char* name;
};
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
