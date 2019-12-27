#pragma once

#include "kl/type_traits.hpp"

#include <boost/variant.hpp>

namespace kl {
namespace detail {

template <typename... F>
struct overloader;

template <typename Head, typename... Tail>
struct overloader<Head, Tail...> : Head, overloader<Tail...>
{
    overloader(Head head, Tail... tail)
        : Head(std::move(head)), overloader<Tail...>(std::move(tail)...)
    {
    }

    using Head::operator();
    using overloader<Tail...>::operator();
};

template <typename F>
struct overloader<F> : F
{
    overloader(F f) : F(std::move(f)) {}

    using F::operator();
};

template <>
struct overloader<>
{
    template <typename U>
    void operator()(U)
    {
        static_assert(always_false_v<U>,
                      "No callable supplied for this overloader");
    }
};

template <typename F, typename... Fs>
struct overload_return_type
{
    static constexpr auto value =
        is_same_v<typename func_traits<F>::return_type,
                  typename func_traits<Fs>::return_type...>;
    static_assert(value, "All supplied callables must return the same type");
    using type = typename func_traits<F>::return_type;
};
} // namespace detail

template <typename ReturnType, typename... Fs>
struct overloader : public boost::static_visitor<ReturnType>,
                    detail::overloader<Fs...>
{
    overloader(Fs... fs) : detail::overloader<Fs...>(std::move(fs)...) {}
};

template <typename ReturnType, typename... Fs>
overloader<ReturnType, Fs...> make_overloader(Fs&&... fs)
{
    return {std::forward<Fs>(fs)...};
}

template <typename Visitable, typename... Fs>
auto match(Visitable&& visitable, Fs&&... fs) ->
    typename detail::overload_return_type<Fs...>::type
{
    using return_type = typename detail::overload_return_type<Fs...>::type;
    auto visitor = make_overloader<return_type>(std::forward<Fs>(fs)...);
    return ::boost::apply_visitor(visitor, std::forward<Visitable>(visitable));
}

// We can't use boost::visitor_ptr along with our overloader since it derives
// from boost::static_visitor which overloader also does
template <typename ReturnType, typename Arg>
struct func_overload
{
    using func_ptr = ReturnType (*)(Arg);

    func_overload(func_ptr func) : func_{func} {}

    using forward_arg =
        std::conditional_t<std::is_reference_v<Arg>, Arg,
                           std::add_lvalue_reference_t<const Arg>>;

    ReturnType operator()(forward_arg arg) const { return func_(arg); }

private:
    func_ptr func_;
};

template <typename ReturnType, typename Arg>
func_overload<ReturnType, Arg> make_func_overload(ReturnType (*func)(Arg))
{
    return {func};
}
} // namespace kl
