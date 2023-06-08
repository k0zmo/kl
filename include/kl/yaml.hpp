#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_set.hpp"
#include "kl/type_traits.hpp"
#include "kl/utility.hpp"
#include "kl/yaml_fwd.hpp"

#include <yaml-cpp/yaml.h>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

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

class default_dump_context
{
public:
    explicit default_dump_context(YAML::Emitter& emitter,
                                  bool skip_null_fields = true)
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

class default_serialize_context
{
public:
    explicit default_serialize_context(bool skip_null_fields = true)
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

template <serialize_context C>
class sequence_builder
{
public:
    explicit sequence_builder(C& ctx)
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
    C& ctx_;
    YAML::Node node_;
};

template <serialize_context C>
class map_builder
{
public:
    explicit map_builder(C& ctx) : ctx_{ctx}, node_{YAML::NodeType::Map} {}

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
    C& ctx_;
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

auto to_sequence(serialize_context auto& ctx)
{
    return detail::sequence_builder{ctx};
}

auto to_map(serialize_context auto& ctx)
{
    return detail::map_builder{ctx};
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

using ::kl::detail::arithmetic;
using ::kl::detail::growable_range;
using ::kl::detail::map_alike;

// encode implementation

// Two overloads below fixes encoding uint8_t as a double-quoted
// hexadecimal value (128 => "\x80\")
void encode(unsigned char v, dump_context auto& ctx)
{
    ctx.emitter() << +v;
}

void encode(signed char v, dump_context auto& ctx)
{
    ctx.emitter() << +v;
}

void encode(const view& v, dump_context auto& ctx)
{
    ctx.emitter() << v.value();
}

void encode(std::nullptr_t, dump_context auto& ctx)
{
    ctx.emitter() << YAML::Null;
}

void encode(const std::string& str, dump_context auto& ctx)
{
    // We repeat encode() for std::string even though yaml-cpp has a native
    // support for it. This is because encode() has higher priority than dump()
    // for supported types and our encode for "range alike" catches this case
    // which is not what we want for the strings.
    ctx.emitter() << str;
}

void encode(const char* str, dump_context auto& ctx)
{
    // Same as above
    ctx.emitter() << str;
}

void encode(const map_alike auto& map, dump_context auto& ctx)
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

void encode(const std::ranges::range auto& rng, dump_context auto& ctx)
{
    ctx.emitter() << YAML::BeginSeq;
    for (const auto& v : rng)
        yaml::dump(v, ctx);
    ctx.emitter() << YAML::EndSeq;
}

void encode(const reflectable auto& refl, dump_context auto& ctx)
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

template <typename Enum> requires std::is_enum_v<Enum>
void encode(const Enum& e, dump_context auto& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        yaml::dump(kl::to_string(e), ctx);
    else
        yaml::dump(underlying_cast(e), ctx);
}

template <typename Enum>
void encode(const enum_set<Enum>& set, dump_context auto& ctx)
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

template <typename... Ts>
void encode(const std::tuple<Ts...>& tuple, dump_context auto& ctx)
{
    ctx.emitter() << YAML::BeginSeq;
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., yaml::dump(std::get<Is>(tuple), ctx));
    }(std::make_index_sequence<sizeof...(Ts)>{});
   ctx.emitter() << YAML::EndSeq;
}

template <typename T>
void encode(const std::optional<T>& opt, dump_context auto& ctx)
{
    if (!opt)
        ctx.emitter() << YAML::Null;
    else
        return yaml::dump(*opt, ctx);
}

// to_yaml implementation

// For all arithmetic types
YAML::Node to_yaml(arithmetic auto value, serialize_context auto&)
{
    return YAML::Node{value};
}

YAML::Node to_yaml(unsigned char value, serialize_context auto&)
{
    return YAML::Node{+value};
}

YAML::Node to_yaml(signed char value, serialize_context auto&)
{
    return YAML::Node{+value};
}

YAML::Node to_yaml(view v, serialize_context auto&)
{
    return v.value();
}

YAML::Node to_yaml(std::nullptr_t, serialize_context auto&)
{
    return YAML::Node{};
}

YAML::Node to_yaml(const std::string& str, serialize_context auto&)
{
    return YAML::Node{str};
}

YAML::Node to_yaml(const char* str, serialize_context auto&)
{
    return YAML::Node{str};
}

// For all T's that quacks like a std::map
template <map_alike M>
YAML::Node to_yaml(const M& map, serialize_context auto& ctx)
{
    static_assert(std::is_constructible_v<std::string, typename M::key_type>,
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
YAML::Node to_yaml(const std::ranges::range auto& rng,
                   serialize_context auto& ctx)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    for (const auto& v : rng)
        arr.push_back(yaml::serialize(v, ctx));
    return arr;
}

// For all T's for which there's a type_info defined
YAML::Node to_yaml(const reflectable auto& refl, serialize_context auto& ctx)
{
    YAML::Node obj{YAML::NodeType::Map};
    ctti::reflect(refl, [&obj, &ctx](auto& field, auto name) {
        if (!ctx.skip_field(name, field))
            obj[name] = yaml::serialize(field, ctx);
    });
    return obj;
}

template <typename Enum> requires std::is_enum_v<Enum>
YAML::Node to_yaml(Enum e, serialize_context auto& ctx)
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

template <typename Enum>
YAML::Node to_yaml(const enum_set<Enum>& set, serialize_context auto& ctx)
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

template <typename... Ts>
YAML::Node to_yaml(const std::tuple<Ts...>& tuple, serialize_context auto& ctx)
{
    YAML::Node arr{YAML::NodeType::Sequence};
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., arr.push_back(yaml::serialize(std::get<Is>(tuple), ctx)));
    }(std::make_index_sequence<sizeof...(Ts)>{});
    return arr;
}

template <typename T>
YAML::Node to_yaml(const std::optional<T>& opt, serialize_context auto& ctx)
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

template <arithmetic A>
void from_yaml(A& out, const YAML::Node& value)
{
    out = from_scalar_yaml<A>(value);
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

template <map_alike M>
void from_yaml(M& out, const YAML::Node& value)
{
    yaml::expect_map(value);

    out.clear();

    for (const auto& obj : value)
    {
        try
        {
            // There's no way to construct K and V directly in the map_alike
            out.emplace(yaml::deserialize<typename M::key_type>(obj.first),
                        yaml::deserialize<typename M::mapped_type>(obj.second));
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

template <growable_range R>
void from_yaml(R& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);

    out.clear();
    if constexpr (requires { out.reserve(0U); })
        out.reserve(value.size());

    for (const auto& item : value)
    {
        try
        {
            // There's no way to construct T directly in the growable_range
            out.push_back(
                yaml::deserialize<typename R::value_type>(item));
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

template <reflectable R>
void reflectable_from_yaml(R& out, const YAML::Node& value)
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
        if (value.size() > ctti::num_fields<R>())
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

template <reflectable R>
void from_yaml(R& out, const YAML::Node& value)
{
    try
    {
        reflectable_from_yaml(out, value);
    }
    catch (deserialize_error& ex)
    {
        std::string msg =
            "error when deserializing type " + std::string(ctti::name<R>());
        ex.add(msg.c_str());
        throw;
    }
}

template <typename Enum> requires std::is_enum_v<Enum>
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

template <typename... Ts>
void from_yaml(std::tuple<Ts...>& out, const YAML::Node& value)
{
    yaml::expect_sequence(value);
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., yaml::deserialize(std::get<Is>(out), yaml::at(value, Is)));
    }(std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
void from_yaml(std::optional<T>& out, const YAML::Node& value)
{
    if (!value || value.IsNull())
        return out.reset();
    // There's no way to construct T directly in the optional
    out = yaml::deserialize<T>(value);
}

} // namespace detail

template <typename T>
std::string dump(const T& obj)
{
    YAML::Emitter emitter;
    default_dump_context ctx{emitter};

    yaml::dump(obj, ctx);
    return {emitter.c_str()};
}

template <typename T>
void dump(const T& obj, dump_context auto& ctx)
{
    using yaml::detail::encode;

    if constexpr (requires { yaml::serializer<T>::encode(obj, ctx); })
        yaml::serializer<T>::encode(obj, ctx);
    else if constexpr (requires { encode(obj, ctx); })
        encode(obj, ctx);
    else if constexpr (requires { ctx.emitter() << obj; })
        ctx.emitter() << obj;
    else
        static_assert(always_false_v<T>,
                      "Cannot dump an instance of type T - no viable "
                      "definition of encode provided");
}

template <typename T>
YAML::Node serialize(const T& obj)
{
    default_serialize_context ctx{};
    return serialize(obj, ctx);
}

template <typename T>
YAML::Node serialize(const T& obj, serialize_context auto& ctx)
{
    using yaml::detail::to_yaml;

    if constexpr (requires { yaml::serializer<T>::to_yaml(obj, ctx); })
        return yaml::serializer<T>::to_yaml(obj, ctx);
    else if constexpr (requires { to_yaml(obj, ctx); })
        return to_yaml(obj, ctx);
    else
        static_assert(always_false_v<T>,
                      "Cannot serialize an instance of type T - no viable "
                      "definition of to_yaml provided");
}

template <typename T>
void deserialize(T& out, const YAML::Node& value)
{
    using yaml::detail::from_yaml;

    if constexpr (requires { yaml::serializer<T>::from_yaml(out, value); })
        yaml::serializer<T>::from_yaml(out, value);
    else if constexpr (requires { from_yaml(out, value); })
        from_yaml(out, value);
    else
        static_assert(always_false_v<T>,
                      "Cannot deserialize an instance of type T - no viable "
                      "definition of from_yaml provided");
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
