#pragma once

#include "kl/type_traits.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_set.hpp"
#include "kl/tuple.hpp"
#include "kl/utility.hpp"

#include <yaml-cpp/yaml.h>

#include <optional>
#include <exception>
#include <string>
#include <string_view>

namespace kl::yaml {

class view
{
public:
    view() = default;
    view(YAML::Node node) : node_{std::move(node)} {}

    const YAML::Node& value() const { return node_; }
    operator const YAML::Node &() const { return value(); }

    explicit operator bool() const { return !!node_; }
    const YAML::Node* operator->() const { return &node_; }
    const YAML::Node& operator*() const { return value(); }

private:
    YAML::Node node_;
};

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
    explicit serialize_context(bool skip_null_fields = true)
        : skip_null_fields_{skip_null_fields}
    {
    }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    bool skip_null_fields_;
};

template <typename T>
YAML::Node serialize(const T& obj);

template <typename T, typename Context>
YAML::Node serialize(const T& obj, Context& ctx);

template <typename T>
void deserialize(T& out, const YAML::Node& value);

// Shorter version of from which can't be overloaded. Only use to invoke
// the from() without providing a bit weird first parameter.
template <typename T>
T deserialize(const YAML::Node& value)
{
    static_assert(std::is_default_constructible_v<T>,
                  "T must be default constructible");

    T out;
    yaml::deserialize(out, value);
    return out;
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

    virtual ~deserialize_error() noexcept;

    const char* what() const noexcept override { return messages_.c_str(); }

    void add(const char* message);

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

    virtual ~parse_error() noexcept;

    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

namespace detail {

inline const YAML::Node& get_null_value()
{
    static const auto null_value = YAML::Node{};
    return null_value;
}
} // namespace detail

// Safely gets the YAML value from the YAML sequence. If provided index is
// out-of-bounds we return a null value.
inline YAML::Node get_value(const YAML::Node& seq, std::size_t idx)
{
    return idx < seq.size() ? seq[idx] : detail::get_null_value();
}

// Safely gets the YAML value from the YAML map. If member of provided name
// is not present we return a null value.
inline YAML::Node get_value(const YAML::Node& map, const char* member_name)
{
    auto query = map[member_name];
    return query ? query : detail::get_null_value();
}

void expect_scalar(const YAML::Node& value);
void expect_sequence(const YAML::Node& value);
void expect_map(const YAML::Node& value);

namespace detail {

std::string type_name(const YAML::Node& value);

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
void encode(const view& v, Context& ctx)
{
    ctx.emitter() << v.value();
}

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

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
void encode(const Enum& e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        yaml::dump(kl::to_string(e), ctx);
    else
        yaml::dump(underlying_cast(e), ctx);
}

template <typename Enum, typename Context>
void encode(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");
    ctx.emitter() << YAML::BeginSeq;
    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (set.test(possible_value))
            yaml::dump(kl::to_string(possible_value), ctx);
    }
    ctx.emitter() << YAML::EndSeq;
}

template <typename Tuple, typename Context, std::size_t... Is>
void encode_tuple(const Tuple& tuple, Context& ctx, std::index_sequence<Is...>)
{
    ctx.emitter() << YAML::BeginSeq;
    (yaml::dump(std::get<Is>(tuple), ctx), ...);
    ctx.emitter() << YAML::EndSeq;
}

template <typename... Ts, typename Context>
void encode(const std::tuple<Ts...>& tuple, Context& ctx)
{
    encode_tuple(tuple, ctx, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T, typename Context>
void encode(const std::optional<T>& opt, Context& ctx)
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
YAML::Node to_yaml(view v, Context& ctx)
{
    return v.value();
}

template <typename Context>
YAML::Node to_yaml(std::nullptr_t, Context&)
{
    return YAML::Node{};
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
    static_assert(std::is_constructible_v<std::string, typename Map::key_type>,
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

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
YAML::Node to_yaml(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
    {
        (void)ctx;
        return YAML::Node{kl::to_string(e)};
    }
    else
    {
        return to_yaml(underlying_cast(e), ctx);
    }
}

template <typename Enum, typename Context>
YAML::Node to_yaml(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");
    YAML::Node arr{YAML::NodeType::Sequence};

    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (set.test(possible_value))
            arr.push_back(to_yaml(possible_value, ctx));
    }

    return arr;
}

template <typename Tuple, typename Context, std::size_t... Is>
YAML::Node tuple_to_yaml(const Tuple& tuple, Context& ctx,
                         index_sequence<Is...>)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    (arr.push_back(yaml::serialize(std::get<Is>(tuple), ctx)), ...);
    return arr;
}

template <typename... Ts, typename Context>
YAML::Node to_yaml(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return tuple_to_yaml(tuple, ctx, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T, typename Context>
YAML::Node to_yaml(const std::optional<T>& opt, Context& ctx)
{
    if (opt)
        return yaml::serialize(*opt, ctx);
    return YAML::Node{};
}

// from_yaml implementation

template <typename T>
T from_scalar_yaml(const YAML::Node& value)
{
    yaml::expect_scalar(value);

    try
    {
        // yaml-cpp conversion of string/scalar to unsigned integer gives wrong
        // result when the scalar represents a negative number
        if constexpr (std::is_unsigned_v<T>)
        {
            // Can YAML scalar be empty?
            if (!value.Scalar().empty() && value.Scalar()[0] == '-')
                throw YAML::TypedBadConversion<T>(value.Mark());
        }

        return value.as<T>();
    }
    catch (const YAML::BadConversion& ex)
    {
        throw deserialize_error{ex.what()};
    }
}

template <typename Arithmetic, enable_if<std::is_arithmetic<Arithmetic>> = true>
void from_yaml(Arithmetic& out, const YAML::Node& value)
{
    out = from_scalar_yaml<Arithmetic>(value);
}

inline void from_yaml(std::string& out, const YAML::Node& value)
{
    yaml::expect_scalar(value);
    out = value.Scalar();
}

inline void from_yaml(std::string_view& out, const YAML::Node& value)
{
    // This variant is unsafe because the lifetime of underlying string is tied
    // to the lifetime of the YAML's scalar value. It may come in handy when
    // writing user-defined `from_yaml` which only need a string_view to do
    // further conversion or when one can guarantee `value` will outlive the
    // returned string_view. Nevertheless, use with caution.
    yaml::expect_scalar(value);
    out = value.Scalar();
}

inline void from_yaml(view& out, const YAML::Node& value)
{
    out = view{value};
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
void from_yaml(Map& out, const YAML::Node& value)
{
    yaml::expect_map(value);

    out.clear();

    for (const auto& obj : value)
    {
        try
        {
            // There's no way to construct K and V directly in the Map
            out.emplace(
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
}

template <typename Vector, enable_if<negation<is_map_alike<Vector>>,
                                     is_vector_alike<Vector>> = true>
void from_yaml(Vector& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);

    out.clear();
    if constexpr (has_reserve_v<Vector>)
        out.reserve(value.size());

    for (const auto& item : value)
    {
        try
        {
            // There's no way to construct T directly in the Vector
            out.push_back(yaml::deserialize<typename Vector::value_type>(item));
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing element " +
                              std::to_string(out.size());
            ex.add(msg.c_str());
            throw;
        }
    }
}

template <typename Reflectable>
void reflectable_from_yaml(Reflectable& out, const YAML::Node& value)
{
    if (value.IsMap())
    {
        ctti::reflect(out, [&value](auto fi) {
            try
            {
                yaml::deserialize(fi.get(), yaml::get_value(value, fi.name()));
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
        ctti::reflect(out, [&value, index = 0U](auto fi) mutable {
            try
            {
                yaml::deserialize(fi.get(), yaml::get_value(value, index));
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
        throw deserialize_error{"type must be a sequence or map but is a " +
                                detail::type_name(value)};
    }
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
void from_yaml(Reflectable& out, const YAML::Node& value)
{
    try
    {
        reflectable_from_yaml(out, value);
    }
    catch (deserialize_error& ex)
    {
        std::string msg = "error when deserializing type " +
                          std::string(ctti::name<Reflectable>());
        ex.add(msg.c_str());
        throw;
    }
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
void from_yaml(Enum& out, const YAML::Node& value)
{
    if constexpr (is_enum_reflectable_v<Enum>)
    {
        yaml::expect_scalar(value);

        if (auto enum_value = kl::from_string<Enum>(value.Scalar()))
        {
            out = *enum_value;
            return;
        }

        throw deserialize_error{"invalid enum value: " + value.Scalar()};
    }
    else
    {
        using underlying_type = std::underlying_type_t<Enum>;
        out = static_cast<Enum>(from_scalar_yaml<underlying_type>(value));
    }
}

template <typename Enum>
void from_yaml(enum_set<Enum>& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);
    out = {};

    for (const auto& v : value)
    {
        const auto e = yaml::deserialize<Enum>(v);
        out |= e;
    }
}

template <typename Tuple, std::size_t... Is>
void tuple_from_yaml(Tuple& out, const YAML::Node& value, index_sequence<Is...>)
{
    (yaml::deserialize(std::get<Is>(out), get_value(value, Is)), ...);
}

template <typename... Ts>
void from_yaml(std::tuple<Ts...>& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);
    tuple_from_yaml(out, value, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
void from_yaml(std::optional<T>& out, const YAML::Node& value)
{
    if (!value || value.IsNull())
        return out.reset();
    // There's no way to construct T directly in the optional
    out = yaml::deserialize<T>(value);
}

template <typename T, typename Context>
void dump(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
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
    -> decltype(yaml::serializer<T>::encode(obj, ctx), void())
{
    yaml::serializer<T>::encode(obj, ctx);
}

template <typename T, typename Context>
YAML::Node serialize(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
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
void deserialize(T& out, const YAML::Node&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of from_yaml provided");
}

template <typename T>
auto deserialize(T& out, const YAML::Node& value, priority_tag<1>)
    -> decltype(from_yaml(out, value), void())
{
    from_yaml(out, value);
}

template <typename T>
auto deserialize(T& out, const YAML::Node& value, priority_tag<2>)
    -> decltype(yaml::serializer<T>::from_yaml(out, value), void())
{
    yaml::serializer<T>::from_yaml(out, value);
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
    serialize_context ctx{};
    return serialize(obj, ctx);
}

template <typename T, typename Context>
YAML::Node serialize(const T& obj, Context& ctx)
{
    return detail::serialize(obj, ctx, priority_tag<2>{});
}

template <typename T>
void deserialize(T& out, const YAML::Node& value)
{
    return detail::deserialize(out, value, priority_tag<2>{});
}
} // namespace kl::yaml

inline YAML::Node operator""_yaml(const char* s, std::size_t)
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
