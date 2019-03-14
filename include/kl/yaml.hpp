#pragma once

#include "kl/type_traits.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_flags.hpp"
#include "kl/tuple.hpp"
#include "kl/utility.hpp"

#include <boost/optional.hpp>
#include <yaml-cpp/yaml.h>

#include <exception>
#include <string>

namespace kl {
namespace yaml {

template <typename T>
struct encoder;

template <typename T>
bool is_null_value(const T& value) { return false; }

template <typename T>
bool is_null_value(const boost::optional<T>& value) { return !value; }

class dump_context
{
public:
    explicit dump_context(YAML::Emitter& emitter, bool skip_null_fields = true)
        : emitter_{emitter}, skip_null_fields_{skip_null_fields}
    {
    }

    YAML::Emitter& emitter() const { return emitter_; }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    YAML::Emitter& emitter_;
    bool skip_null_fields_;
};

template <typename T>
std::string dump(const T& obj);

template <typename T, typename Context>
void dump(const T& obj, Context& ctx);

struct parse_error : std::exception
{
    explicit parse_error(const char* message)
        : parse_error{std::string(message)}
    {
    }

    explicit parse_error(std::string message) noexcept
        : message_{std::move(message)}
    {
    }

    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

namespace detail {

KL_HAS_TYPEDEF_HELPER(value_type)
KL_HAS_TYPEDEF_HELPER(iterator)
KL_HAS_TYPEDEF_HELPER(mapped_type)
KL_HAS_TYPEDEF_HELPER(key_type)

KL_VALID_EXPR_HELPER(has_c_str, std::declval<T&>().c_str())

template <typename T>
struct is_map_alike
    : conjunction<
        has_value_type<T>,
        has_iterator<T>,
        has_mapped_type<T>,
        has_key_type<T>> {};

template <typename T>
struct is_vector_alike
    : conjunction<
        has_value_type<T>,
        has_iterator<T>,
        negation<has_c_str<T>>> {};

template <typename Context>
void encode(std::nullptr_t, Context& ctx)
{
    ctx.emitter() << YAML::Null;
}

template <typename Map, typename Context, enable_if<is_map_alike<Map>> = true>
void encode(const Map& map, Context& ctx)
{
    ctx.emitter() << YAML::BeginMap;
    for (const auto& kv : map)
    {
        if (!ctx.skip_field(kv.first, kv.second))
        {
            ctx.emitter() << YAML::Key;
            ctx.emitter() << kv.first;
            ctx.emitter() << YAML::Value;
            yaml::dump(kv.second, ctx);
        }
    }
    ctx.emitter() << YAML::EndMap;
}

template <
    typename Vector, typename Context,
    enable_if<negation<is_map_alike<Vector>>, is_vector_alike<Vector>> = true>
void encode(const Vector& vec, Context& ctx)
{
    ctx.emitter() << YAML::BeginSeq;
    for (const auto& v : vec)
        yaml::dump(v, ctx);
    ctx.emitter() << YAML::EndSeq;
}

template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
void encode(const Reflectable& refl, Context& ctx)
{
    ctx.emitter() << YAML::BeginMap;
    ctti::reflect(refl, [&ctx](auto fi) {
        if (!ctx.skip_field(fi.name(), fi.get()))
        {
            ctx.emitter() << YAML::Key;
            ctx.emitter() << fi.name();
            ctx.emitter() << YAML::Value;
            yaml::dump(fi.get(), ctx);
        }
    });
    ctx.emitter() << YAML::EndMap;
}

template <typename Enum, typename Context>
void encode_enum(Enum e, Context& ctx, std::false_type /*is_enum_reflectable*/)
{
    yaml::dump(underlying_cast(e), ctx);
}

template <typename Enum, typename Context>
void encode_enum(Enum e, Context& ctx, std::true_type /*is_enum_reflectable*/)
{
    yaml::dump(kl::to_string(e), ctx);
}

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
void encode(const Enum& e, Context& ctx)
{
    encode_enum(e, ctx, is_enum_reflectable<Enum>{});
}

template <typename Enum, typename Context>
void encode(const enum_flags<Enum>& flags, Context& ctx)
{
    static_assert(is_enum_reflectable<Enum>::value,
                  "Only flags of reflectable enums are supported");
    ctx.emitter() << YAML::BeginSeq;
    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (flags.test(possible_value))
            yaml::dump(kl::to_string(possible_value), ctx);
    }
    ctx.emitter() << YAML::EndSeq;
}

template <typename Tuple, typename Context, std::size_t... Is>
void encode_tuple(const Tuple& tuple, Context& ctx, std::index_sequence<Is...>)
{
    ctx.emitter() << YAML::BeginSeq;
    using swallow = std::initializer_list<int>;
    (void)swallow{(yaml::dump(std::get<Is>(tuple), ctx), 0)...};
    ctx.emitter() << YAML::EndSeq;
}

template <typename... Ts, typename Context>
void encode(const std::tuple<Ts...>& tuple, Context& ctx)
{
    encode_tuple(tuple, ctx, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T, typename Context>
void encode(const boost::optional<T>& opt, Context& ctx)
{
    if (!opt)
        ctx.emitter() << YAML::Null;
    else
        return yaml::dump(*opt, ctx);
}

template <typename T, typename Context>
void dump(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot dump an instance of type T - no viable "
                  "definition of encode provided");
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(ctx.emitter() << obj, void())
{
    ctx.emitter() << obj;
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(encode(obj, ctx), void())
{
    encode(obj, ctx);
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<3>)
    -> decltype(yaml::encoder<T>::encode(obj, ctx), void())
{
    yaml::encoder<T>::encode(obj, ctx);
}
} // namespace detail

template <typename T>
std::string dump(const T& obj)
{
    YAML::Emitter emitter;
    dump_context ctx{emitter};

    yaml::dump(obj, ctx);
    return {emitter.c_str()};
}

template <typename T, typename Context>
void dump(const T& obj, Context& ctx)
{
    detail::dump(obj, ctx, priority_tag<3>{});
}
} // namespace yaml
} // namespace kl

inline YAML::Node operator""_yaml(const char* s, std::size_t len)
{
    try
    {
        return YAML::Load(s);
    }
    catch (const YAML::Exception& ex)
    {
        throw kl::yaml::parse_error{ex.what()};
    }
}
