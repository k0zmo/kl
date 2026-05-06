#pragma once

#include "kl/serialization.hpp"
#include "kl/utility.hpp"
#include "kl/yaml_fwd.hpp"

#include <yaml-cpp/yaml.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace kl::yaml {

namespace detail {

struct yaml_stream_backend;
struct yaml_tree_backend;

} // namespace detail

struct stream_tag : serialization::backend_tag<detail::yaml_stream_backend> {};
struct tree_tag   : serialization::backend_tag<detail::yaml_tree_backend>   {};


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

class dump_context : public stream_tag
{
public:
    explicit dump_context(YAML::Emitter& emitter, bool skip_null_fields = true)
        : emitter_{emitter}, skip_null_fields_{skip_null_fields}
    {
    }

    YAML::Emitter& emitter() const { return emitter_; }

    template <typename Value>
    bool skip_null_value(const Value& value)
    {
        return skip_null_fields_ && serialization::is_null_value(value);
    }

private:
    YAML::Emitter& emitter_;
    bool skip_null_fields_;
};

class serialize_context : public tree_tag
{
public:
    explicit serialize_context(bool skip_null_fields = true)
        : skip_null_fields_{skip_null_fields}
    {
    }

    template <typename Value>
    bool skip_null_value(const Value& value)
    {
        return skip_null_fields_ && serialization::is_null_value(value);
    }

private:
    bool skip_null_fields_;
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
        catch (serialization::deserialize_error& ex)
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
        catch (serialization::deserialize_error& ex)
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

struct yaml_tree_backend
{
    using value_type = YAML::Node;

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

} // namespace detail
} // namespace kl::yaml

namespace kl::serialization {

// dump_adl implementation for more complex types (like seqs, maps, reflectable structs and enums)
template <typename T, typename Context>
auto dump_adl(yaml::stream_tag, const T& value, Context& ctx)
    -> decltype(detail::dump_adl<yaml::detail::yaml_stream_backend>(value, ctx), void())
{
    detail::dump_adl<yaml::detail::yaml_stream_backend>(value, ctx);
}

// Two overloads below fixes encoding uint8_t as a double-quoted
// hexadecimal value (128 => "\x80\")
template <typename Context>
void dump_adl(yaml::stream_tag, unsigned char v, Context& ctx)
{
    ctx.emitter() << +v;
}

template <typename Context>
void dump_adl(yaml::stream_tag, signed char v, Context& ctx)
{
    ctx.emitter() << +v;
}

template <typename Context>
void dump_adl(yaml::stream_tag, const yaml::view& v, Context& ctx)
{
    ctx.emitter() << v.value();
}

template <typename Context>
void dump_adl(yaml::stream_tag, std::nullptr_t, Context& ctx)
{
    ctx.emitter() << YAML::Null;
}

template <typename Context>
void dump_adl(yaml::stream_tag, const std::string& str, Context& ctx)
{
    // We repeat dump_adl() for std::string even though yaml-cpp has a native
    // support for it. This is because dump_adl() has higher priority than dump()
    // for supported types and our dump_adl for "range alike" catches this case
    // which is not what we want for the strings. Also, we don't provide const
    // char* overload since yaml-cpp's overload calls std::string variant in the
    // end.
    ctx.emitter() << str;
}

// serialize_adl implementation for more complex types (like seqs, maps, reflectable structs and enums)
template <typename T, typename Context>
auto serialize_adl(yaml::tree_tag, const T& value, Context& ctx)
    -> decltype(detail::serialize_adl<yaml::detail::yaml_tree_backend>(value, ctx))
{
    return detail::serialize_adl<yaml::detail::yaml_tree_backend>(value, ctx);
}

// serialize_adl implementations for simple types

template <typename Arithmetic, typename Context, enable_if<std::is_arithmetic<Arithmetic>> = true>
YAML::Node serialize_adl(yaml::tree_tag, Arithmetic value, Context&)
{
    return YAML::Node{value};
}

template <typename Context>
YAML::Node serialize_adl(yaml::tree_tag, unsigned char value, Context&)
{
    return YAML::Node{+value};
}

template <typename Context>
YAML::Node serialize_adl(yaml::tree_tag, signed char value, Context&)
{
    return YAML::Node{+value};
}

template <typename Context>
YAML::Node serialize_adl(yaml::tree_tag, yaml::view v, Context&)
{
    return v.value();
}

template <typename Context>
YAML::Node serialize_adl(yaml::tree_tag, std::nullptr_t, Context&)
{
    return YAML::Node{};
}

template <typename Context>
YAML::Node serialize_adl(yaml::tree_tag, const std::string& str, Context&)
{
    return YAML::Node{str};
}

template <typename Context>
YAML::Node serialize_adl(yaml::tree_tag, const char* str, Context&)
{
    return YAML::Node{str};
}

// deserialize_adl implementation for more complex types (like seqs, maps, reflectable structs and enums)
template <typename T>
auto deserialize_adl(yaml::tree_tag, T& out, const YAML::Node& value)
    -> decltype(detail::deserialize_adl<yaml::detail::yaml_tree_backend>(out, value), void())
{
    detail::deserialize_adl<yaml::detail::yaml_tree_backend>(out, value);
}

// deserialize_adl implementations for simple types

template <typename Arithmetic, enable_if<std::is_arithmetic<Arithmetic>> = true>
void deserialize_adl(yaml::tree_tag, Arithmetic& out, const YAML::Node& value)
{
    yaml::detail::expect_scalar(value);

    try
    {
        out = value.template as<Arithmetic>();
    }
    catch (const YAML::BadConversion& ex)
    {
        throw serialization::deserialize_error{ex.what()};
    }
}

inline void deserialize_adl(yaml::tree_tag, std::string& out, const YAML::Node& value)
{
    yaml::detail::expect_scalar(value);
    out = value.Scalar();
}

inline void deserialize_adl(yaml::tree_tag, std::string_view& out, const YAML::Node& value)
{
    // This variant is unsafe because the lifetime of underlying string is tied
    // to the lifetime of the YAML's scalar value. It may come in handy when
    // writing user-defined `deserialize_adl` which only need a string_view to do
    // further conversion or when one can guarantee `value` will outlive the
    // returned string_view. Nevertheless, use with caution.
    yaml::detail::expect_scalar(value);
    out = value.Scalar();
}

inline void deserialize_adl(yaml::tree_tag, yaml::view& out, const YAML::Node& value)
{
    out = yaml::view{value};
}

template <>
struct backend_traits<yaml::detail::yaml_stream_backend>
{
private:
    template <typename T, typename Context>
    static auto dump_impl(const T& obj, Context& ctx, priority_tag<0>)
        -> decltype(ctx.emitter() << obj, void())
    {
        ctx.emitter() << obj;
    }

    template <typename T, typename Context>
    static auto dump_impl(const T& obj, Context& ctx, priority_tag<1>)
        -> decltype(dump_adl(yaml::stream_tag{}, obj, ctx), void())
    {
        dump_adl(yaml::stream_tag{}, obj, ctx);
    }

public:
    template <typename T, typename Context>
    static auto dump(const T& obj, Context& ctx)
        -> decltype(dump_impl(obj, ctx, priority_tag<1>{}), void())
    {
        dump_impl(obj, ctx, priority_tag<1>{});
    }
};

template <>
struct backend_traits<yaml::detail::yaml_tree_backend>
{
    template <typename T, typename Context>
    static auto serialize(const T& obj, Context& ctx)
        -> decltype(serialize_adl(yaml::tree_tag{}, obj, ctx))
    {
        return serialize_adl(yaml::tree_tag{}, obj, ctx);
    }

    template <typename T>
    static auto deserialize(T& out, const YAML::Node& value)
        -> decltype(deserialize_adl(yaml::tree_tag{}, out, value), void())
    {
        deserialize_adl(yaml::tree_tag{}, out, value);
    }
};

template <>
struct backend_for_value<YAML::Node>
{
    using type = yaml::detail::yaml_tree_backend;
};

} // namespace kl::serialization

// Top level functions

namespace kl::yaml {

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
    serialization::detail::dump_with_backend<stream_tag>(obj, ctx);
}

template <typename T>
YAML::Node serialize(const T& obj)
{
    serialize_context ctx{};
    return yaml::serialize(obj, ctx);
}

template <typename T, typename Context>
YAML::Node serialize(const T& obj, Context& ctx)
{
    return serialization::detail::serialize_with_backend<tree_tag>(obj, ctx);
}

template <typename T>
void deserialize(T& out, const YAML::Node& value)
{
    return serialization::deserialize(out, value);
}

// Shorter version of deserialize which can't be overloaded. Only use to invoke
// the deserialize() without providing a bit weird first parameter.
template <typename T>
T deserialize(const YAML::Node& value)
{
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
    return serialization::deserialize<T>(value);
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
        throw kl::serialization::parse_error{ex.what()};
    }
}
