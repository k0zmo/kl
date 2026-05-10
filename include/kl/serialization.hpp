#pragma once

#include "kl/serialization_fwd.hpp"
#include "kl/detail/serialization.hpp"
#include "kl/type_traits.hpp"
#include "kl/utility.hpp"

#include <type_traits>

namespace kl::serialization {

/*
   Customization contract
   ======================
  
   There are two user-facing customization mechanisms:
  
   1. Specialize `kl::serialization::serializer<T>`.
      This has higher dispatch priority than backend ADL. If `serializer<T>`
      provides `dump()`, `serialize()`, or `deserialize()`, that member is selected
      before any `dump_adl()`, `serialize_adl()`, or `deserialize_adl()` overload.
  
   2. Provide backend ADL overloads.
      The backend tag is the first argument and identifies the concrete backend
      family. For example, JSON uses `kl::json::stream_tag` for `dump_adl()` and
      `kl::json::tree_tag` for `serialize_adl()`/`deserialize_adl()`. YAML follows
      the same stream_tag/tree_tag split.
  
          void dump_adl(kl::json::stream_tag, const T&, Context&);
          rapidjson::Value serialize_adl(kl::json::tree_tag, const T&, Context&);
          void deserialize_adl(kl::json::tree_tag, T&, const rapidjson::Value&, Context&);
  
      Backend-independent ADL can be written by templating the tag:
  
          template <typename Tag, typename Context>
          auto serialize_adl(Tag, const T&, Context&);
          template <typename Tag, typename Value, typename Context>
          void deserialize_adl(Tag, T&, const Value&, Context&);
  
   Direct `kl::serialization::dump()/serialize()/deserialize()` calls require a
   context carrying `backend_type`. Built-in contexts do this by inheriting the
   public backend tags. Backend-specific wrappers such as `kl::json::dump()`,
   `kl::json::serialize()`, and `kl::json::deserialize()` force the backend
   themselves, so custom contexts used through those wrappers do not need to
   expose backend_type.
*/

namespace detail {

template <typename T, typename Backend, typename Context>
void dump(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot dump an instance of type T - no viable definition of "
                  "dump_adl or serializer provided");
}

template <typename T, typename Backend, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(backend_traits<Backend>::dump(obj, ctx), void())
{
    backend_traits<Backend>::dump(obj, ctx);
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
    -> decltype(backend_traits<Backend>::serialize(obj, ctx))
{
    return backend_traits<Backend>::serialize(obj, ctx);
}

template <typename T, typename Backend, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(serializer<T>::serialize(obj, ctx))
{
    return serializer<T>::serialize(obj, ctx);
}

template <typename T, typename Backend, typename Value, typename Context>
void deserialize(T&, const Value&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of deserialize_adl or serializer provided");
}

template <typename T, typename Backend, typename Value, typename Context>
auto deserialize(T& out, const Value& value, Context& ctx, priority_tag<1>)
    -> decltype(backend_traits<Backend>::deserialize(out, value, ctx), void())
{
    backend_traits<Backend>::deserialize(out, value, ctx);
}

template <typename T, typename Backend, typename Value, typename Context>
auto deserialize(T& out, const Value& value, Context& ctx, priority_tag<2>)
    -> decltype(serializer<T>::deserialize(out, value, ctx), void())
{
    serializer<T>::deserialize(out, value, ctx);
}

} // namespace detail

template <typename T, typename Context>
void dump(const T& obj, Context& ctx)
{
    using backend = detail::backend_t<Context>;
    detail::dump<T, backend>(obj, ctx, priority_tag<2>{});
}

namespace detail {

template <typename BackendIdentity, typename T, typename Context>
void dump_with_backend(const T& obj, Context& ctx)
{
    using backend = backend_t<BackendIdentity>;
    detail::dump<T, backend>(obj, ctx, priority_tag<2>{});
}

} // namespace detail

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx)
    -> decltype(detail::serialize<T, detail::backend_t<Context>>(
        obj, ctx, priority_tag<2>{}))
{
    using backend = detail::backend_t<Context>;
    return detail::serialize<T, backend>(obj, ctx, priority_tag<2>{});
}

namespace detail {

template <typename BackendIdentity, typename T, typename Context>
auto serialize_with_backend(const T& obj, Context& ctx)
    -> decltype(detail::serialize<T, backend_t<BackendIdentity>>(obj, ctx, priority_tag<2>{}))
{
    using backend = backend_t<BackendIdentity>;
    return detail::serialize<T, backend>(obj, ctx, priority_tag<2>{});
}

template <typename BackendIdentity, typename T, typename Value, typename Context>
void deserialize_with_backend(T& out, const Value& value, Context& ctx)
{
    using backend = backend_t<BackendIdentity>;
    detail::deserialize<T, backend>(out, value, ctx, priority_tag<2>{});
}

} // namespace detail

template <typename T, typename Value, typename Context>
void deserialize(T& out, const Value& value, Context& ctx)
{
    using backend = detail::backend_t<Context>;
    detail::deserialize<T, backend>(out, value, ctx, priority_tag<2>{});
}

template <typename T, typename Value, typename Context>
T deserialize(const Value& value, Context& ctx)
{
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
    T out;
    serialization::deserialize(out, value, ctx);
    return out;
}

} // namespace kl::serialization
