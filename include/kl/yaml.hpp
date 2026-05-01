#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/detail/serialization.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_set.hpp"
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

void expect_scalar(const YAML::Node& value);
void expect_sequence(const YAML::Node& value);
void expect_map(const YAML::Node& value);
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
        detail::expect_map(node_);
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
        detail::expect_sequence(node_);
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
            std::string msg = "error when deserializing element " + std::to_string(index_);
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

using ::kl::detail::is_map_alike;
using ::kl::detail::is_range;

struct yaml_stream_backend
{
    template <typename T, typename Context>
    static void dump(const T& value, Context& ctx)
    {
        yaml::dump(value, ctx);
    }

    // Map stuff

    template <typename Context>
    static void begin_map(Context& ctx)
    {
        ctx.emitter() << YAML::BeginMap;
    }

    template <typename Context>
    static void end_map(Context& ctx)
    {
        ctx.emitter() << YAML::EndMap;
    }

    template <typename Key, typename Context>
    static void write_key(const Key& key, Context& ctx)
    {
        ctx.emitter() << YAML::Key;
        ctx.emitter() << key;
        ctx.emitter() << YAML::Value;
    }

    // Sequence stuff

    template <typename Context>
    static void begin_sequence(Context& ctx)
    {
        ctx.emitter() << YAML::BeginSeq;
    }

    template <typename Context>
    static void end_sequence(Context& ctx)
    {
        ctx.emitter() << YAML::EndSeq;
    }
};

// dump_adl implementation for more complex types (like sequence, map, reflectable structs and enums)
template <typename T, typename Context>
auto dump_adl(const T& value, Context& ctx)
    -> decltype(serialization::detail::dump_adl<yaml_stream_backend>(value, ctx), void())
{
    serialization::detail::dump_adl<yaml_stream_backend>(value, ctx);
}

// Two overloads below fixes encoding uint8_t as a double-quoted
// hexadecimal value (128 => "\x80\")
template <typename Context>
void dump_adl(unsigned char v, Context& ctx)
{
    ctx.emitter() << +v;
}

template <typename Context>
void dump_adl(signed char v, Context& ctx)
{
    ctx.emitter() << +v;
}

template <typename Context>
void dump_adl(const view& v, Context& ctx)
{
    ctx.emitter() << v.value();
}

template <typename Context>
void dump_adl(std::nullptr_t, Context& ctx)
{
    ctx.emitter() << YAML::Null;
}

template <typename Context>
void dump_adl(const std::string& str, Context& ctx)
{
    // We repeat dump_adl() for std::string even though yaml-cpp has a native
    // support for it. This is because dump_adl() has higher priority than dump()
    // for supported types and our dump_adl for "range alike" catches this case
    // which is not what we want for the strings. Also, we don't provide const
    // char* overload since yaml-cpp's overload calls std::string variant in the
    // end.
    ctx.emitter() << str;
}

struct yaml_tree_backend
{
    using value_type = YAML::Node;
    using deserialize_error = yaml::deserialize_error;

    template <typename T, typename Context>
    static value_type serialize(const T& value, Context& ctx)
    {
        return yaml::serialize(value, ctx);
    }

    template <typename T>
    static void deserialize(T& out, const value_type& value)
    {
        yaml::deserialize(out, value);
    }

    // Map stuff

    static value_type make_map() { return value_type{YAML::NodeType::Map}; }
    static void expect_map(const value_type& value) { detail::expect_map(value); }
    static bool is_map(const value_type& value) { return value.IsMap(); }

    template <typename Key, typename Context>
    static void add_field(value_type& out, const Key& key, value_type value,
                          Context&)
    {
        out[key] = std::move(value);
    }

    template <typename Visitor>
    static void for_each_field(const value_type& value, Visitor&& visitor)
    {
        for (const auto& obj : value)
            visitor(obj.first, obj.second);
    }

    // Sequence stuff

    static value_type make_sequence() { return value_type{YAML::NodeType::Sequence}; }
    static void expect_sequence(const value_type& value) { detail::expect_sequence(value); }
    static bool is_sequence(const value_type& value) { return value.IsSequence(); }

    template <typename Context>
    static void add_element(value_type& out, value_type value, Context&)
    {
        out.push_back(std::move(value));
    }

    template <typename Visitor>
    static void for_each_element(const value_type& value, Visitor&& visitor)
    {
        for (const auto& item : value)
            visitor(item);
    }

    // Rest of the stuff

    static bool is_null(const value_type& value) { return !value || value.IsNull(); }
    static std::size_t size(const value_type& value) { return value.size(); }

    template <typename Key>
    static value_type at_field(const value_type& value, const Key& name)
    {
        return yaml::at(value, name);
    }

    static value_type at_index(const value_type& value, std::size_t index)
    {
        return yaml::at(value, index);
    }

    static std::string type_name(const value_type& value)
    {
        return detail::type_name(value);
    }
};

// serialize_adl implementation for more complex types (like sequence, map, reflectable structs and enums)
template <typename T, typename Context>
auto serialize_adl(const T& value, Context& ctx)
    -> decltype(serialization::detail::serialize_adl<yaml_tree_backend>(value, ctx))
{
    return serialization::detail::serialize_adl<yaml_tree_backend>(value, ctx);
}

// serialize_adl implementations for simple types

template <typename Arithmetic, typename Context, enable_if<std::is_arithmetic<Arithmetic>> = true>
YAML::Node serialize_adl(Arithmetic value, Context&)
{
    return YAML::Node{value};
}

template <typename Context>
YAML::Node serialize_adl(unsigned char value, Context&)
{
    return YAML::Node{+value};
}

template <typename Context>
YAML::Node serialize_adl(signed char value, Context&)
{
    return YAML::Node{+value};
}

template <typename Context>
YAML::Node serialize_adl(view v, Context&)
{
    return v.value();
}

template <typename Context>
YAML::Node serialize_adl(std::nullptr_t, Context&)
{
    return YAML::Node{};
}

template <typename Context>
YAML::Node serialize_adl(const std::string& str, Context&)
{
    return YAML::Node{str};
}

template <typename Context>
YAML::Node serialize_adl(const char* str, Context&)
{
    return YAML::Node{str};
}

// from_yaml implementation

template <typename T>
T from_scalar_yaml(const YAML::Node& value)
{
    detail::expect_scalar(value);

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
    detail::expect_scalar(value);
    out = value.Scalar();
}

inline void from_yaml(std::string_view& out, const YAML::Node& value)
{
    // This variant is unsafe because the lifetime of underlying string is tied
    // to the lifetime of the YAML's scalar value. It may come in handy when
    // writing user-defined `from_yaml` which only need a string_view to do
    // further conversion or when one can guarantee `value` will outlive the
    // returned string_view. Nevertheless, use with caution.
    detail::expect_scalar(value);
    out = value.Scalar();
}

inline void from_yaml(view& out, const YAML::Node& value)
{
    out = view{value};
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
void from_yaml(Map& out, const YAML::Node& value)
{
    serialization::detail::deserialize_map<yaml_tree_backend>(out, value);
}

template <typename GrowableRange,
          enable_if<std::negation<is_map_alike<GrowableRange>>,
                    is_range<GrowableRange>> = true>
void from_yaml(GrowableRange& out, const YAML::Node& value)
{
    serialization::detail::deserialize_range<yaml_tree_backend>(out, value);
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
void from_yaml(Reflectable& out, const YAML::Node& value)
{
    serialization::detail::deserialize_reflectable<yaml_tree_backend>(out, value);
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
void from_yaml(Enum& out, const YAML::Node& value)
{
    serialization::detail::deserialize_enum<yaml_tree_backend>(out, value);
}

template <typename Enum>
void from_yaml(enum_set<Enum>& out, const YAML::Node& value)
{
    serialization::detail::deserialize_enum_set<yaml_tree_backend>(out, value);
}

template <typename... Ts>
void from_yaml(std::tuple<Ts...>& out, const YAML::Node& value)
{
    serialization::detail::deserialize_tuple<yaml_tree_backend>(out, value);
}

template <typename T>
void from_yaml(std::optional<T>& out, const YAML::Node& value)
{
    serialization::detail::deserialize_optional<yaml_tree_backend>(out, value);
}

template <typename T, typename Context>
void dump(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot dump an instance of type T - no viable "
                  "definition of dump_adl provided");
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(ctx.emitter() << obj, void())
{
    ctx.emitter() << obj;
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(dump_adl(obj, ctx), void())
{
    dump_adl(obj, ctx);
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<3>)
    -> decltype(yaml::serializer<T>::dump(obj, ctx), void())
{
    yaml::serializer<T>::dump(obj, ctx);
}

template <typename T, typename Context>
YAML::Node serialize(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot serialize an instance of type T - no viable "
                  "definition of serialize_adl provided");
    return {}; // Keeps compiler happy
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(serialize_adl(obj, ctx))
{
    return serialize_adl(obj, ctx);
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(yaml::serializer<T>::serialize(obj, ctx))
{
    return yaml::serializer<T>::serialize(obj, ctx);
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

// Shorter version of deserialize which can't be overloaded. Only use to invoke
// the deserialize() without providing a bit weird first parameter.
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
