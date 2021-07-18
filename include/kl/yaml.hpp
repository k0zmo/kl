#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_set.hpp"
#include "kl/utility.hpp"
#include "kl/yaml_fwd.hpp"

#include <yaml-cpp/yaml.h>

#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace kl::yaml {

class view
{
public:
    view() = default;
    view(YAML::Node node) : node_{std::move(node)} {}

    const YAML::Node& value() const { return node_; }
    operator const YAML::Node&() const { return value(); }

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
inline YAML::Node at(const YAML::Node& seq, std::size_t idx)
{
    return idx < seq.size() ? seq[idx] : detail::get_null_value();
}

// Safely gets the YAML value from the YAML map. If member of provided name
// is not present we return a null value.
template <typename Key>
inline YAML::Node at(const YAML::Node& map, const Key& member_name)
{
    auto query = map[member_name];
    return query ? query : detail::get_null_value();
}

void expect_scalar(const YAML::Node& value);
void expect_sequence(const YAML::Node& value);
void expect_map(const YAML::Node& value);

namespace detail {

template <typename Context>
class sequence_builder
{
public:
    explicit sequence_builder(Context& ctx)
        : ctx_{ctx}, node_{YAML::NodeType::Sequence}
    {
    }

    template <typename T>
    sequence_builder& add(const T& value)
    {
        return add(yaml::serialize(value, ctx_));
    }

    sequence_builder& add(YAML::Node v)
    {
        node_.push_back(std::move(v));
        return *this;
    }

    YAML::Node done() { return std::move(node_); }
    operator YAML::Node() { return std::move(node_); }

private:
    Context& ctx_;
    YAML::Node node_;
};

template <typename Context>
class map_builder
{
public:
    explicit map_builder(Context& ctx) : ctx_{ctx}, node_{YAML::NodeType::Map}
    {
    }

    template <typename T, typename Key>
    map_builder& add(const Key& member_name, const T& value)
    {
        return add(member_name, yaml::serialize(value, ctx_));
    }

    template <typename Key>
    map_builder& add(const Key& member_name, YAML::Node v)
    {
        node_[member_name] = std::move(v);
        return *this;
    }

    YAML::Node done() { return std::move(node_); }
    operator YAML::Node() { return std::move(node_); }

private:
    Context& ctx_;
    YAML::Node node_;
};

class map_extractor
{
public:
    explicit map_extractor(const YAML::Node& node) : node_{node}
    {
        yaml::expect_map(node_);
    }

    template <typename T, typename Key>
    map_extractor& extract(const Key& member_name, T& out)
    {
        try
        {
            yaml::deserialize(out, yaml::at(node_, member_name));
            return *this;
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing field ";
            msg += member_name;
            ex.add(msg.c_str());
            throw;
        }
    }

private:
    const YAML::Node& node_;
};

class sequence_extractor
{
public:
    explicit sequence_extractor(const YAML::Node& node) : node_{node}
    {
        yaml::expect_sequence(node_);
    }

    template <typename T>
    sequence_extractor& extract(T& out, unsigned index)
    {
        index_ = index;
        return extract<T>(out);
    }

    template <typename T>
    sequence_extractor& extract(T& out)
    {
        try
        {
            yaml::deserialize(out, yaml::at(node_, index_));
            ++index_;
            return *this;
        }
        catch (deserialize_error& ex)
        {
            std::string msg =
                "error when deserializing element " + std::to_string(index_);
            ex.add(msg.c_str());
            throw;
        }
    }

private:
    const YAML::Node& node_;
    unsigned index_{};
};
} // namespace detail

template <typename Context>
auto to_sequence(Context& ctx)
{
    return detail::sequence_builder<Context>{ctx};
}

template <typename Context>
auto to_map(Context& ctx)
{
    return detail::map_builder<Context>{ctx};
}

inline auto from_sequence(const YAML::Node& node)
{
    return detail::sequence_extractor{node};
}

inline auto from_map(const YAML::Node& node)
{
    return detail::map_extractor{node};
}

namespace detail {

std::string type_name(const YAML::Node& value);

using ::kl::detail::has_reserve_v;
using ::kl::detail::is_growable_range;
using ::kl::detail::is_map_alike;
using ::kl::detail::is_range;

// encode implementation

// Two overloads below fixes encoding uint8_t as a double-quoted
// hexadecimal value (128 => "\x80\")
template <typename Context>
void encode(unsigned char v, Context& ctx)
{
    ctx.emitter() << +v;
}

template <typename Context>
void encode(signed char v, Context& ctx)
{
    ctx.emitter() << +v;
}

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

template <typename Context>
void encode(const std::string& str, Context& ctx)
{
    // We repeat encode() for std::string even though yaml-cpp has a native
    // support for it. This is because encode() has higher priority than dump()
    // for supported types and our encode for "range alike" catches this case
    // which is not what we want for the strings. Also, we don't provide const
    // char* overload since yaml-cpp's overload calls std::string variant in the
    // end.
    ctx.emitter() << str;
}

template <typename Map, typename Context, enable_if<is_map_alike<Map>> = true>
void encode(const Map& map, Context& ctx)
{
    ctx.emitter() << YAML::BeginMap;
    for (const auto& [key, value] : map)
    {
        if (!ctx.skip_field(key, value))
        {
            ctx.emitter() << YAML::Key;
            ctx.emitter() << key;
            ctx.emitter() << YAML::Value;
            yaml::dump(value, ctx);
        }
    }
    ctx.emitter() << YAML::EndMap;
}

template <typename Range, typename Context,
          enable_if<std::negation<is_map_alike<Range>>, is_range<Range>> = true>
void encode(const Range& rng, Context& ctx)
{
    ctx.emitter() << YAML::BeginSeq;
    for (const auto& v : rng)
        yaml::dump(v, ctx);
    ctx.emitter() << YAML::EndSeq;
}

template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
void encode(const Reflectable& refl, Context& ctx)
{
    ctx.emitter() << YAML::BeginMap;
    ctti::reflect(refl, [&ctx](auto& field, auto name) {
        if (!ctx.skip_field(name, field))
        {
            ctx.emitter() << YAML::Key;
            ctx.emitter() << name;
            ctx.emitter() << YAML::Value;
            yaml::dump(field, ctx);
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
    for (const auto possible_value : reflect<Enum>().values())
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
    encode_tuple(tuple, ctx, std::make_index_sequence<sizeof...(Ts)>{});
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
YAML::Node to_yaml(unsigned char value, Context&)
{
    return YAML::Node{+value};
}

template <typename Context>
YAML::Node to_yaml(signed char value, Context&)
{
    return YAML::Node{+value};
}

template <typename Context>
YAML::Node to_yaml(view v, Context&)
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
    for (const auto& [key, value] : map)
    {
        if (!ctx.skip_field(key, value))
            obj[key] = yaml::serialize(value, ctx);
    }
    return obj;
}

// For all T's that quacks like a range
template <typename Range, typename Context,
          enable_if<std::negation<is_map_alike<Range>>, is_range<Range>> = true>
YAML::Node to_yaml(const Range& rng, Context& ctx)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    for (const auto& v : rng)
        arr.push_back(yaml::serialize(v, ctx));
    return arr;
}

// For all T's for which there's a type_info defined
template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
YAML::Node to_yaml(const Reflectable& refl, Context& ctx)
{
    YAML::Node obj{YAML::NodeType::Map};
    ctti::reflect(refl, [&obj, &ctx](auto& field, auto name) {
        if (!ctx.skip_field(name, field))
            obj[name] = yaml::serialize(field, ctx);
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

    for (const auto possible_value : reflect<Enum>().values())
    {
        if (set.test(possible_value))
            arr.push_back(to_yaml(possible_value, ctx));
    }

    return arr;
}

template <typename Tuple, typename Context, std::size_t... Is>
YAML::Node tuple_to_yaml(const Tuple& tuple, Context& ctx,
                         std::index_sequence<Is...>)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    (arr.push_back(yaml::serialize(std::get<Is>(tuple), ctx)), ...);
    return arr;
}

template <typename... Ts, typename Context>
YAML::Node to_yaml(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return tuple_to_yaml(tuple, ctx, std::make_index_sequence<sizeof...(Ts)>{});
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

template <typename GrowableRange,
          enable_if<std::negation<is_map_alike<GrowableRange>>,
                    is_range<GrowableRange>> = true>
void from_yaml(GrowableRange& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);

    out.clear();
    if constexpr (has_reserve_v<GrowableRange>)
        out.reserve(value.size());

    for (const auto& item : value)
    {
        try
        {
            // There's no way to construct T directly in the GrowableRange
            out.push_back(
                yaml::deserialize<typename GrowableRange::value_type>(item));
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
        ctti::reflect(out, [&value](auto& field, auto name) {
            try
            {
                yaml::deserialize(field, yaml::at(value, name));
            }
            catch (deserialize_error& ex)
            {
                std::string msg =
                    "error when deserializing field " + std::string(name);
                ex.add(msg.c_str());
                throw;
            }
        });
    }
    else if (value.IsSequence())
    {
        if (value.size() > ctti::num_fields<Reflectable>())
        {
            throw deserialize_error{"sequence size is greater than "
                                    "declared struct's field "
                                    "count"};
        }
        ctti::reflect(out, [&value, index = 0U](auto& field, auto) mutable {
            try
            {
                yaml::deserialize(field, yaml::at(value, index));
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
void tuple_from_yaml(Tuple& out, const YAML::Node& value,
                     std::index_sequence<Is...>)
{
    (yaml::deserialize(std::get<Is>(out), yaml::at(value, Is)), ...);
}

template <typename... Ts>
void from_yaml(std::tuple<Ts...>& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);
    tuple_from_yaml(out, value, std::make_index_sequence<sizeof...(Ts)>{});
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
void deserialize(T&, const YAML::Node&, priority_tag<0>)
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
