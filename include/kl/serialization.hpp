#pragma once

#include "kl/detail/serialization.hpp"
#include "kl/type_traits.hpp"
#include "kl/utility.hpp"

#include <type_traits>

namespace kl::serialization {

template <typename T>
struct serializer;

template <typename Backend>
struct backend_traits;

template <typename Value>
struct backend_for_value;

namespace detail {

template <typename Context>
using backend_t = typename Context::backend_type;

template <typename Value>
using backend_for_value_t =
    typename backend_for_value<remove_cvref_t<Value>>::type;

template <typename T, typename Backend, typename Context>
void dump(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot dump an instance of type T - no viable definition of "
                  "dump_adl or serializer provided");
}

template <typename T, typename Backend, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(backend_traits<Backend>::dump_adl(obj, ctx), void())
{
    backend_traits<Backend>::dump_adl(obj, ctx);
}

template <typename T, typename Backend, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(serializer<T>::dump(obj, ctx), void())
{
    serializer<T>::dump(obj, ctx);
}

template <typename T, typename Backend, typename Context>
auto serialize(const T&, Context&, priority_tag<0>)
    -> typename Backend::value_type
{
    static_assert(always_false_v<T>,
                  "Cannot serialize an instance of type T - no viable "
                  "definition of serialize_adl or serializer provided");
    return {};
}

template <typename T, typename Backend, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(backend_traits<Backend>::serialize_adl(obj, ctx))
{
    return backend_traits<Backend>::serialize_adl(obj, ctx);
}

template <typename T, typename Backend, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(serializer<T>::serialize(obj, ctx))
{
    return serializer<T>::serialize(obj, ctx);
}

template <typename T, typename Backend, typename Value>
void deserialize(T&, const Value&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of deserialize_adl or serializer provided");
}

template <typename T, typename Backend, typename Value>
auto deserialize(T& out, const Value& value, priority_tag<1>)
    -> decltype(backend_traits<Backend>::deserialize_adl(out, value), void())
{
    backend_traits<Backend>::deserialize_adl(out, value);
}

template <typename T, typename Backend, typename Value>
auto deserialize(T& out, const Value& value, priority_tag<2>)
    -> decltype(serializer<T>::deserialize(out, value), void())
{
    serializer<T>::deserialize(out, value);
}

} // namespace detail

template <typename T, typename Context>
void dump(const T& obj, Context& ctx)
{
    using backend = detail::backend_t<Context>;
    detail::dump<T, backend>(obj, ctx, priority_tag<2>{});
}

template <typename Backend, typename T, typename Context>
void dump_with_backend(const T& obj, Context& ctx)
{
    detail::dump<T, Backend>(obj, ctx, priority_tag<2>{});
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx)
    -> decltype(detail::serialize<T, detail::backend_t<Context>>(
        obj, ctx, priority_tag<2>{}))
{
    using backend = detail::backend_t<Context>;
    return detail::serialize<T, backend>(obj, ctx, priority_tag<2>{});
}

template <typename Backend, typename T, typename Context>
auto serialize_with_backend(const T& obj, Context& ctx)
    -> decltype(detail::serialize<T, Backend>(obj, ctx, priority_tag<2>{}))
{
    return detail::serialize<T, Backend>(obj, ctx, priority_tag<2>{});
}

template <typename T, typename Value>
void deserialize(T& out, const Value& value)
{
    using backend = detail::backend_for_value_t<Value>;
    detail::deserialize<T, backend>(out, value, priority_tag<2>{});
}

template <typename T, typename Value>
T deserialize(const Value& value)
{
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
    T out;
    serialization::deserialize(out, value);
    return out;
}

} // namespace kl::serialization
