#pragma once

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

#define KL_TOKEN_PASTEx(x, y) x##y
#define KL_TOKEN_PASTE(x, y) KL_TOKEN_PASTEx(x, y)

#define KL_DEFER_INTERNAL1(lname, aname, ...)                                  \
    auto lname = [&]() { __VA_ARGS__; };                                       \
    kl::detail::scope_exit_t<decltype(lname)> aname(lname);

#define KL_DEFER_INTERNAL2(ctr, ...)                                           \
    KL_DEFER_INTERNAL1(KL_TOKEN_PASTE(defer_func_, ctr),                       \
                       KL_TOKEN_PASTE(defer_instance_, ctr), __VA_ARGS__)

#define KL_DEFER(...) KL_DEFER_INTERNAL2(__COUNTER__, __VA_ARGS__)
