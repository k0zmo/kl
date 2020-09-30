#pragma once

#include "kl/detail/macros.hpp"

namespace kl {
namespace detail {

template <typename Fun>
class scope_exit_t
{
    Fun& fun_;

public:
    scope_exit_t(Fun& fun) : fun_(fun) {}
    ~scope_exit_t() { fun_(); }
};
} // namespace detail
} // namespace kl

#define KL_DEFER_INTERNAL1(lname, aname, ...)                                  \
    auto lname = [&]() { __VA_ARGS__; };                                       \
    kl::detail::scope_exit_t<decltype(lname)> aname(lname);

#define KL_DEFER_INTERNAL2(counter_, ...)                                      \
    KL_DEFER_INTERNAL1(KL_CONCAT(kl_defer_func, counter_),                     \
                       KL_CONCAT(kl_defer_instance, counter_), __VA_ARGS__)

#define KL_DEFER(...) KL_DEFER_INTERNAL2(__COUNTER__, __VA_ARGS__)
