#pragma once

#include <type_traits>
#include <utility>
#include <variant>

namespace kl {

// Workaround for:
// https://developercommunity.visualstudio.com/t/Run-Time-Check-Failure-possibly-related/10005513?q=rtc1
// Should be fixed in next release according to
// https://developercommunity.visualstudio.com/t/Runtime-stack-corruption-using-std::visi/346200#T-N10160953
#if defined(_MSC_VER)
#define KL_OVERLOADER_EMPTY_BASE_CLASS __declspec(empty_bases)
#else
#define KL_OVERLOADER_EMPTY_BASE_CLASS
#endif

template <typename... Fs>
struct KL_OVERLOADER_EMPTY_BASE_CLASS overloader : Fs...
{
    using Fs::operator()...;
};

template <typename... Fs>
overloader(Fs...) -> overloader<Fs...>;

// Wrapper over a single-argument function so that a free-function can be used
// along the lambdas as part of the overloader list
template <typename ReturnType, typename Arg>
struct func_overload
{
    using func_ptr = ReturnType (*)(Arg);

    explicit func_overload(func_ptr func) : func_{func} {}

    using forward_arg =
        std::conditional_t<std::is_reference_v<Arg>, Arg,
                           std::add_lvalue_reference_t<const Arg>>;

    ReturnType operator()(forward_arg arg) const { return func_(arg); }

private:
    func_ptr func_;
};

template <typename ReturnType, typename Arg>
func_overload(ReturnType (*)(Arg)) -> func_overload<ReturnType, Arg>;

template <typename Visitable, typename... Fs>
auto match(Visitable&& visitable, Fs&&... fs)
{
    return std::visit(overloader{std::forward<Fs>(fs)...}, visitable);
}

} // namespace kl
