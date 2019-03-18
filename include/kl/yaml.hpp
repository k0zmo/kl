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

KL_DEFINE_ENUM_REFLECTOR(YAML, NodeType::value,
                         (Undefined, Null, Scalar, Sequence, Map))

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

template <typename T>
struct serializer;

class serialize_context
{
public:
    explicit serialize_context(YAML::Node& node, bool skip_null_fields = true)
        : node_{node}, skip_null_fields_{skip_null_fields}
    {
    }

    YAML::Node& node() { return node_; }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    YAML::Node& node_;
    bool skip_null_fields_;
};

template <typename T>
YAML::Node serialize(const T& obj);

template <typename T, typename Context>
YAML::Node serialize(const T& obj, Context& ctx);

template <typename T>
T deserialize(type_t<T>, const YAML::Node& value);

// Shorter version of from which can't be overloaded. Only use to invoke
// the from() without providing a bit weird first parameter.
template <typename T>
T deserialize(const YAML::Node& value)
{
    // TODO replace with variable template type<T>
    return yaml::deserialize(type_t<T>{}, value);
}

struct deserialize_error : std::exception
{
    explicit deserialize_error(const char* message)
        : deserialize_error(std::string(message))
    {
    }
    explicit deserialize_error(std::string message) noexcept
        : messages_(std::move(message))
    {
    }

    const char* what() const noexcept override { return messages_.c_str(); }

    void add(const char* message)
    {
        messages_.insert(end(messages_), '\n');
        messages_.append(message);
    }

private:
    std::string messages_;
};

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
KL_VALID_EXPR_HELPER(has_reserve, std::declval<T&>().reserve(0U))

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

// encode implementation

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

// to_yaml implementation

// For all arithmetic types
template <typename Arithmetic, typename Context,
          enable_if<std::is_arithmetic<Arithmetic>> = true>
YAML::Node to_yaml(Arithmetic value, Context&)
{
    return YAML::Node{value};
}

template <typename Context>
YAML::Node to_yaml(const std::string& str, Context&)
{
    return YAML::Node{str};
}

template <typename Context>
YAML::Node to_yaml(const char* str, Context&)
{
    return YAML::Node{str};
}

// For all T's that quacks like a std::map
template <typename Map, typename Context, enable_if<is_map_alike<Map>> = true>
YAML::Node to_yaml(const Map& map, Context& ctx)
{
    static_assert(
        std::is_constructible<std::string, typename Map::key_type>::value,
        "std::string must be constructible from the Map's key type");

    YAML::Node obj{YAML::NodeType::Map};
    for (const auto& kv : map)
    {
        if (!ctx.skip_field(kv.first, kv.second))
            obj[kv.first] = yaml::serialize(kv.second, ctx);
    }
    return obj;
}

// For all T's that quacks like a std::vector
template <
    typename Vector, typename Context,
    enable_if<negation<is_map_alike<Vector>>, is_vector_alike<Vector>> = true>
YAML::Node to_yaml(const Vector& vec, Context& ctx)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    for (const auto& v : vec)
        arr.push_back(yaml::serialize(v, ctx));
    return arr;
}

// For all T's for which there's a type_info defined
template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
YAML::Node to_yaml(const Reflectable& refl, Context& ctx)
{
    YAML::Node obj{YAML::NodeType::Map};
    ctti::reflect(refl, [&obj, &ctx](auto fi) {
        if (!ctx.skip_field(fi.name(), fi.get()))
            obj[fi.name()] = yaml::serialize(fi.get(), ctx);
    });
    return obj;
}

template <typename Enum, typename Context>
YAML::Node enum_to_yaml(Enum e, Context& ctx,
                        std::true_type /*is_enum_reflectable*/)
{
    return YAML::Node{kl::to_string(e)};
}

template <typename Enum, typename Context>
YAML::Node enum_to_yaml(Enum e, Context& ctx,
                        std::false_type /*is_enum_reflectable*/)
{
    return to_yaml(underlying_cast(e), ctx);
}

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
YAML::Node to_yaml(Enum e, Context& ctx)
{
    return enum_to_yaml(e, ctx, is_enum_reflectable<Enum>{});
}

template <typename Enum, typename Context>
YAML::Node to_yaml(const enum_flags<Enum>& flags, Context& ctx)
{
    static_assert(is_enum_reflectable<Enum>::value,
                  "Only flags of reflectable enums are supported");
    YAML::Node arr{YAML::NodeType::Sequence};

    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (flags.test(possible_value))
            arr.push_back(to_yaml(possible_value, ctx));
    }

    return arr;
}

template <typename Tuple, typename Context, std::size_t... Is>
YAML::Node tuple_to_yaml(const Tuple& tuple, Context& ctx,
                         index_sequence<Is...>)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    using swallow = std::initializer_list<int>;
    (void)swallow{
        (arr.push_back(yaml::serialize(std::get<Is>(tuple), ctx)), 0)...};
    return arr;
}

template <typename... Ts, typename Context>
YAML::Node to_yaml(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return tuple_to_yaml(tuple, ctx, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T, typename Context>
YAML::Node to_yaml(const boost::optional<T>& opt, Context& ctx)
{
    if (opt)
        return yaml::serialize(*opt, ctx);
    return YAML::Node{};
}

// from_yaml implementation

inline std::string yaml_type_name(const YAML::Node& value)
{
    return kl::to_string(value.Type());
}

template <typename T>
T from_scalar_yaml(const YAML::Node& value)
{
    if (!value.IsScalar())
        throw deserialize_error{"type must be a scalar but is " +
                                yaml_type_name(value)};

    try
    {
        return value.as<T>();
    }
    catch (const YAML::BadConversion& ex)
    {
        throw deserialize_error{ex.what()};
    }
}

template <typename Arithmetic, enable_if<std::is_arithmetic<Arithmetic>> = true>
Arithmetic from_yaml(type_t<Arithmetic>, const YAML::Node& value)
{
    return from_scalar_yaml<Arithmetic>(value);
}

inline std::string from_yaml(type_t<std::string>, const YAML::Node& value)
{
    if (!value.IsScalar())
        throw deserialize_error{"type must be a scalar but is " +
                                yaml_type_name(value)};
    return value.Scalar();
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
Map from_yaml(type_t<Map>, const YAML::Node& value)
{
    if (!value.IsMap())
        throw deserialize_error{"type must be a map but is " +
                                yaml_type_name(value)};

    Map ret{};

    for (const auto& obj : value)
    {
        try
        {
            ret.emplace(
                yaml::deserialize<typename Map::key_type>(obj.first),
                yaml::deserialize<typename Map::mapped_type>(obj.second));
        }
        catch (deserialize_error& ex)
        {
            std::string msg =
                "error when deserializing field " + obj.first.Scalar();
            ex.add(msg.c_str());
            throw;
        }
    }

    return ret;
}

template <typename Vector>
void vector_reserve(Vector&, std::size_t, std::false_type)
{
}

template <typename Vector>
void vector_reserve(Vector& vec, std::size_t size, std::true_type)
{
    vec.reserve(size);
}

template <typename Vector, enable_if<negation<is_map_alike<Vector>>,
                                     is_vector_alike<Vector>> = true>
Vector from_yaml(type_t<Vector>, const YAML::Node& value)
{
    if (!value.IsSequence())
        throw deserialize_error{"type must be a sequence but is " +
                                yaml_type_name(value)};

    Vector ret{};
    vector_reserve(ret, value.size(), has_reserve<Vector>{});

    for (const auto& item : value)
    {
        try
        {
            ret.push_back(yaml::deserialize<typename Vector::value_type>(item));
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing element " +
                              std::to_string(ret.size());
            ex.add(msg.c_str());
            throw;
        }
    }

    return ret;
}

template <typename Reflectable>
Reflectable reflectable_from_yaml(const YAML::Node& value)
{
    Reflectable refl{};

    if (value.IsMap())
    {
        ctti::reflect(refl, [&value](auto fi) {
            using field_type = typename decltype(fi)::type;

            try
            {
                const auto& query = value[fi.name()];
                if (query)
                    fi.get() = yaml::deserialize<field_type>(query);
                else
                    fi.get() = yaml::deserialize<field_type>({});
            }
            catch (deserialize_error& ex)
            {
                std::string msg =
                    "error when deserializing field " + std::string(fi.name());
                ex.add(msg.c_str());
                throw;
            }
        });
    }
    else if (value.IsSequence())
    {
        if (value.size() > ctti::total_num_fields<Reflectable>())
        {
            throw deserialize_error{"sequence size is greater than "
                                    "declared struct's field "
                                    "count"};
        }
        ctti::reflect(refl, [&value, index = 0U](auto fi) mutable {
            using field_type = typename decltype(fi)::type;

            try
            {
                if (index < value.size())
                    fi.get() = yaml::deserialize<field_type>(value[index]);
                else
                    fi.get() = yaml::deserialize<field_type>({});
                ++index;
            }
            catch (deserialize_error& ex)
            {
                std::string msg =
                    "error when deserializing element " + std::to_string(index);
                ex.add(msg.c_str());
                throw;
            }
        });
    }
    else
    {
        throw deserialize_error{"type must be a sequence or map but is " +
                                yaml_type_name(value)};
    }

    return refl;
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
Reflectable from_yaml(type_t<Reflectable>, const YAML::Node& value)
{
    static_assert(std::is_default_constructible<Reflectable>::value,
                  "Reflectable must be default constructible");

    try
    {
        return reflectable_from_yaml<Reflectable>(value);
    }
    catch (deserialize_error& ex)
    {
        std::string msg = "error when deserializing type " +
                          std::string(ctti::name<Reflectable>());
        ex.add(msg.c_str());
        throw;
    }
}

template <typename Enum>
Enum enum_from_yaml(const YAML::Node& value,
                    std::false_type /*is_enum_reflectable*/)
{
    using underlying_type = std::underlying_type_t<Enum>;
    return static_cast<Enum>(from_scalar_yaml<underlying_type>(value));
}

template <typename Enum>
Enum enum_from_yaml(const YAML::Node& value,
                    std::true_type /*is_enum_reflectable*/)
{
    if (!value.IsScalar())
        throw deserialize_error{"type must be a scalar but is " +
                                yaml_type_name(value)};
    if (auto enum_value = kl::from_string<Enum>(value.Scalar()))
        return enum_value.get();
    throw deserialize_error{"invalid enum value: " + value.Scalar()};
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
Enum from_yaml(type_t<Enum>, const YAML::Node& value)
{
    return enum_from_yaml<Enum>(value, is_enum_reflectable<Enum>{});
}

template <typename Enum>
enum_flags<Enum> from_yaml(type_t<enum_flags<Enum>>, const YAML::Node& value)
{
    if (!value.IsSequence())
        throw deserialize_error{"type must be a sequnce but is " +
                                yaml_type_name(value)};

    enum_flags<Enum> ret{};

    for (const auto& v : value)
    {
        const auto e = yaml::deserialize<Enum>(v);
        ret |= e;
    }

    return ret;
}

// Safely gets the YAML value from the sequence of YAML values. If provided
// index is out-of-bounds we return a null value.
inline const YAML::Node safe_get_value(const YAML::Node& seq, std::size_t idx)
{
    static const auto null_value = YAML::Node{};
    return idx >= seq.size() ? null_value : seq[idx];
}

template <typename Tuple, std::size_t... Is>
static Tuple tuple_from_yaml(const YAML::Node& value, index_sequence<Is...>)
{
    return std::make_tuple(yaml::deserialize<std::tuple_element_t<Is, Tuple>>(
        safe_get_value(value, Is))...);
}

template <typename... Ts>
std::tuple<Ts...> from_yaml(type_t<std::tuple<Ts...>>, const YAML::Node& value)
{
    if (!value.IsSequence())
        throw deserialize_error{"type must be a sequence but is " +
                                yaml_type_name(value)};

    return tuple_from_yaml<std::tuple<Ts...>>(
        value, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
boost::optional<T> from_yaml(type_t<boost::optional<T>>,
                             const YAML::Node& value)
{
    if (!value || value.IsNull())
        return {};
    return yaml::deserialize<T>(value);
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

template <typename T, typename Context>
YAML::Node serialize(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot serialize an instance of type T - no viable "
                  "definition of to_yaml provided");
    return {}; // Keeps compiler happy
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(to_yaml(obj, ctx))
{
    return to_yaml(obj, ctx);
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(yaml::serializer<T>::to_yaml(obj, ctx))
{
    return yaml::serializer<T>::to_yaml(obj, ctx);
}

template <typename T>
T deserialize(const YAML::Node&, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of from_yaml provided");
    return T{}; // Keeps compiler happy
}

template <typename T>
auto deserialize(const YAML::Node& value, priority_tag<1>)
    -> decltype(from_yaml(std::declval<type_t<T>>(), value))
{
    // TODO replace with variable template type<T>
    return from_yaml(type_t<T>{}, value);
}

template <typename T>
auto deserialize(const YAML::Node& value, priority_tag<2>)
    -> decltype(yaml::serializer<T>::from_yaml(value))
{
    return yaml::serializer<T>::from_yaml(value);
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

template <typename T>
YAML::Node serialize(const T& obj)
{
    YAML::Node node;
    serialize_context ctx{node};
    return serialize(obj, ctx);
}

template <typename T, typename Context>
YAML::Node serialize(const T& obj, Context& ctx)
{
    return detail::serialize(obj, ctx, priority_tag<2>{});
}

template <typename T>
T deserialize(type_t<T>, const YAML::Node& value)
{
    return detail::deserialize<T>(value, priority_tag<2>{});
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
