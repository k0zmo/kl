#pragma once

#include "kl/detail/serialization.hpp"
#include "kl/type_traits.hpp"
#include "kl/utility.hpp"

#include <optional>
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
          void deserialize_adl(kl::json::tree_tag, T&, const rapidjson::Value&);
  
      Backend-independent ADL can be written by templating the tag:
  
          template <typename Tag, typename Context>
          auto serialize_adl(Tag, const T&, Context&);
  
   Direct `kl::serialization::dump()/serialize()` calls require a context carrying
   `backend_type`. Built-in contexts do this by inheriting the public backend tags.
   Backend-specific wrappers such as `kl::json::dump()`/`serialize()` and
   `kl::yaml::dump()`/`serialize()` force the backend themselves, so custom contexts
   used through those wrappers do not need to expose backend_type.
*/
template <typename T>
struct serializer;

template <typename T>
struct optional_traits
{
    static bool is_null_value(const T&) { return false; }
};

template <typename T>
struct optional_traits<std::optional<T>>
{
    static bool is_null_value(const std::optional<T>& opt) { return !opt; }
};

template <typename T>
bool is_null_value(const T& t)
{
    return optional_traits<T>::is_null_value(t);
}

template <typename Backend>
struct backend_traits;

template <typename Backend>
struct backend_tag
{
    using backend_type = Backend;
};

template <typename Value>
struct backend_for_value;

namespace detail {

template <typename BackendIdentity>
using backend_t = typename BackendIdentity::backend_type;

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

template <typename T, typename Backend, typename Value>
void deserialize(T&, const Value&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of deserialize_adl or serializer provided");
}

template <typename T, typename Backend, typename Value>
auto deserialize(T& out, const Value& value, priority_tag<1>)
    -> decltype(backend_traits<Backend>::deserialize(out, value), void())
{
    backend_traits<Backend>::deserialize(out, value);
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

} // namespace detail

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
