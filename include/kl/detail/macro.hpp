#pragma once

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>

// Stringize the argument: arg -> "arg"
#define KL_STRINGIZE(arg_) BOOST_PP_STRINGIZE(arg_)

// Concatenates two identifiers
#define KL_CONCAT(a_, b_) BOOST_PP_CAT(a_, b_)

// Makes sure arg is a tuple (works for tuples and single arg)
#define KL_ARG_TO_TUPLE(arg_) (BOOST_PP_REMOVE_PARENS(arg_))

// Turns a, b, c into (a, b, c)
#define KL_VARIADIC_TO_TUPLE(...) BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)

// Returns the size of the tuple
#define KL_TUPLE_SIZE(tuple_) BOOST_PP_TUPLE_SIZE(tuple_)

// Returns the size of the tuple decreased by one
#define KL_TUPLE_SIZE_MINUS_ONE(tuple_) BOOST_PP_DEC(KL_TUPLE_SIZE(tuple_))

// Returns the tuple element of given index
#define KL_TUPLE_ELEM(index_, tuple_) BOOST_PP_TUPLE_ELEM(index_, tuple_)

// If given tuple has at least two elements returns the 2nd one, otherwise
// returns the 1st one
#define KL_TUPLE_SECOND_OR_FIRST_ELEM(tuple_)                                  \
    BOOST_PP_IF(KL_TUPLE_SIZE_MINUS_ONE(tuple_), KL_TUPLE_ELEM(1, tuple_),     \
                KL_TUPLE_ELEM(0, tuple_))

// Does the for each for tuple. Body is a macro invoked with (tuple_[i])
#define KL_TUPLE_FOR_EACH(tuple_, body_)                                       \
    BOOST_PP_REPEAT(KL_TUPLE_SIZE(tuple_), KL_TUPLE_LOOP_INVOKER,              \
                    (tuple_, body_))

// Does the for each for tuple. Body is a macro invoked with (arg_, tuple_[i])
#define KL_TUPLE_FOR_EACH2(arg_, tuple_, body_)                                \
    BOOST_PP_REPEAT(KL_TUPLE_SIZE(tuple_), KL_TUPLE_LOOP_INVOKER2,             \
                    (arg_, tuple_, body_))

// tuple_macro is (tuple_, body_)
#define KL_TUPLE_LOOP_INVOKER(_, index_, tuple_macro_)                         \
    KL_TUPLE_ELEM(1, tuple_macro_)                                             \
    (KL_TUPLE_ELEM(index_, KL_TUPLE_ELEM(0, tuple_macro_)))

// tuple_macro_ is (arg_, tuple_, body_)
#define KL_TUPLE_LOOP_INVOKER2(_, index_, tuple_macro_)                        \
    KL_TUPLE_ELEM(2, tuple_macro_)                                             \
    (KL_TUPLE_ELEM(0, tuple_macro_),                                           \
     KL_TUPLE_ELEM(index_, KL_TUPLE_ELEM(1, tuple_macro_)))
