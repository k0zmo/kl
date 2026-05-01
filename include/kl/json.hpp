#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/detail/serialization.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_set.hpp"
#include "kl/json_fwd.hpp"
#include "kl/utility.hpp"

// Undefine Win32 macro
#if defined(GetObject)
#  undef GetObject
#endif

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/error/error.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kl::json {

class view
{
public:
    view() : value_{nullptr} {}
    view(const rapidjson::Value& value) : value_{&value} {}

    const rapidjson::Value& value() const { return *value_; }
    operator const rapidjson::Value &() const { return value(); }

    explicit operator bool() const { return value_ != nullptr; }
    const rapidjson::Value* operator->() const { return value_; }
    const rapidjson::Value& operator*() const { return value(); }

private:
    const rapidjson::Value* value_;
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

template <typename Writer>
class dump_context
{
public:
    using writer_type = Writer;

    explicit dump_context(Writer& writer, bool skip_null_fields = true)
        : writer_{writer}, skip_null_fields_{skip_null_fields}
    {
    }

    Writer& writer() const { return writer_; }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    Writer& writer_;
    bool skip_null_fields_;
};

class owning_serialize_context
{
public:
    explicit owning_serialize_context(bool skip_null_fields = true)
        : skip_null_fields_{skip_null_fields}
    {
    }

    json::allocator& allocator() { return alloc_; }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    json::allocator alloc_;
    bool skip_null_fields_;
};

class serialize_context
{
public:
    explicit serialize_context(rapidjson::Document& doc,
                               bool skip_null_fields = true)
        : serialize_context{doc.GetAllocator(), skip_null_fields}
    {
    }

    explicit serialize_context(json::allocator& alloc,
                               bool skip_null_fields = true)
        : alloc_{alloc}, skip_null_fields_{skip_null_fields}
    {
    }

    json::allocator& allocator() { return alloc_; }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    json::allocator& alloc_;
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

inline const rapidjson::Value& get_null_value()
{
    static const auto null_value = rapidjson::Value{};
    return null_value;
}

void expect_integral(const rapidjson::Value& value);
void expect_number(const rapidjson::Value& value);
void expect_boolean(const rapidjson::Value& value);
void expect_string(const rapidjson::Value& value);
void expect_object(const rapidjson::Value& value);
void expect_array(const rapidjson::Value& value);
} // namespace detail

// Safely gets the JSON value from the JSON array. If provided index is
// out-of-bounds we return a null value.
inline const rapidjson::Value& at(const rapidjson::Value::ConstArray& arr, rapidjson::SizeType idx)
{
    return idx < arr.Size() ? arr[idx] : detail::get_null_value();
}

// Safely gets the JSON value from the JSON object. If member of provided name
// is not present we return a null value.
inline const rapidjson::Value& at(rapidjson::Value::ConstObject obj, const char* member_name)
{
    const auto it = obj.FindMember(member_name);
    return it != obj.end() ? it->value : detail::get_null_value();
}

namespace detail {

template <typename Context>
class array_builder
{
public:
    explicit array_builder(Context& ctx) noexcept
        : ctx_{ctx}, value_{rapidjson::kArrayType}
    {
    }

    template <typename T>
    array_builder& add(const T& value)
    {
        return add(json::serialize(value, ctx_));
    }

    array_builder& add(rapidjson::Value v)
    {
        value_.PushBack(std::move(v), ctx_.allocator());
        return *this;
    }

    rapidjson::Value done() { return std::move(value_); }
    operator rapidjson::Value() { return std::move(value_); }

private:
    Context& ctx_;
    rapidjson::Value value_;
};

template <typename Context>
class object_builder
{
public:
    explicit object_builder(Context& ctx) noexcept
        : ctx_{ctx}, value_{rapidjson::kObjectType}
    {
    }

    // Uses rapidjson::Value constructor for constant string which does not make
    // a copy of string.
    template <typename T>
    object_builder& add(std::string_view member_name, const T& value)
    {
        return add(member_name, json::serialize(value, ctx_));
    }

    // Uses rapidjson::Value constructor for constant string which does not
    // make a copy of string.
    object_builder& add(std::string_view member_name, rapidjson::Value value)
    {
        rapidjson::Value name{member_name.data(),
                              static_cast<rapidjson::SizeType>(member_name.size())};
        return add(std::move(name), std::move(value));
    }

    // Uses rapidjson::Value constructor for dynamic string,
    // making a copy of `member_name`.
    template <typename T>
    object_builder& add_dynamic_name(std::string_view member_name, const T& value)
    {
        return add_dynamic_name(member_name, json::serialize(value, ctx_));
    }

    // Uses rapidjson::Value constructor for dynamic string,
    // making a copy of `member_name`.
    object_builder& add_dynamic_name(std::string_view member_name,
                                     rapidjson::Value value)
    {
        rapidjson::Value name{member_name.data(),
                              static_cast<rapidjson::SizeType>(member_name.size()),
                              ctx_.allocator()};
        return add(std::move(name), std::move(value));
    }

    object_builder& add(rapidjson::Value member_name, rapidjson::Value value)
    {
        value_.AddMember(std::move(member_name), std::move(value), ctx_.allocator());
        return *this;
    }

    rapidjson::Value done() { return std::move(value_); }
    operator rapidjson::Value() { return std::move(value_); }

private:
    Context& ctx_;
    rapidjson::Value value_;
};

class object_extractor
{
public:
    explicit object_extractor(const rapidjson::Value& value) : value_{value}
    {
        detail::expect_object(value_);
    }

    template <typename T>
    object_extractor& extract(const char* member_name, T& out)
    {
        try
        {
            json::deserialize(out, json::at(value_.GetObject(), member_name));
            return *this;
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing field " + std::string(member_name);
            ex.add(msg.c_str());
            throw;
        }
    }

private:
    const rapidjson::Value& value_;
};

class array_extractor
{
public:
    explicit array_extractor(const rapidjson::Value& value) : value_{value}
    {
        detail::expect_array(value_);
    }

    template <typename T>
    array_extractor& extract(T& out, unsigned index)
    {
        index_ = index;
        return extract<T>(out);
    }

    template <typename T>
    array_extractor& extract(T& out)
    {
        try
        {
            json::deserialize(out, json::at(value_.GetArray(), index_));
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
    const rapidjson::Value& value_;
    unsigned index_{};
};
} // namespace detail

template <typename Context>
auto to_array(Context& ctx)
{
    return detail::array_builder<Context>{ctx};
}

template <typename Context>
auto to_object(Context& ctx)
{
    return detail::object_builder<Context>{ctx};
}

inline auto from_array(const rapidjson::Value& value)
{
    return detail::array_extractor{value};
}

inline auto from_object(const rapidjson::Value& value)
{
    return detail::object_extractor{value};
}

namespace detail {

std::string type_name(const rapidjson::Value& value);

using ::kl::detail::is_growable_range;
using ::kl::detail::is_map_alike;
using ::kl::detail::is_range;

struct json_stream_backend
{
    // Trampoline from the kl::serialization back to json "world"
    template <typename T, typename Context>
    static void dump(const T& value, Context& ctx)
    {
        json::dump(value, ctx);
    }

    // Map stuff

    template <typename Context>
    static void begin_map(Context& ctx)
    {
        ctx.writer().StartObject();
    }

    template <typename Context>
    static void end_map(Context& ctx)
    {
        ctx.writer().EndObject();
    }

    template <typename Key, typename Context>
    static void write_key(const Key& key, Context& ctx)
    {
        write_key_impl(key, ctx);
    }

    // Sequence stuff

    template <typename Context>
    static void begin_sequence(Context& ctx)
    {
        ctx.writer().StartArray();
    }

    template <typename Context>
    static void end_sequence(Context& ctx)
    {
        ctx.writer().EndArray();
    }

private:
    template <typename Key, typename Context>
    static void write_key_impl(const Key& key, Context& ctx)
    {
        ctx.writer().Key(key.c_str(), static_cast<rapidjson::SizeType>(key.size()));
    }

    template <typename Context>
    static void write_key_impl(const char* key, Context& ctx)
    {
        ctx.writer().Key(key);
    }
};

// dump_adl implementation for more complex types (like sequence, map, reflectable structs and enums)
template <typename T, typename Context>
auto dump_adl(const T& value, Context& ctx)
    -> decltype(serialization::detail::dump_adl<json_stream_backend>(value, ctx), void())
{
    serialization::detail::dump_adl<json_stream_backend>(value, ctx);
}

// dump_adl implementations for simple types

template <typename Context>
void dump_adl(view v, Context& ctx)
{
    v.value().Accept(ctx.writer());
}

template <typename Context>
void dump_adl(std::nullptr_t, Context& ctx)
{
    ctx.writer().Null();
}

template <typename Context>
void dump_adl(bool b, Context& ctx)
{
    ctx.writer().Bool(b);
}

template <typename Context>
void dump_adl(int i, Context& ctx)
{
    ctx.writer().Int(i);
}

template <typename Context>
void dump_adl(unsigned int u, Context& ctx)
{
    ctx.writer().Uint(u);
}

template <typename Context>
void dump_adl(std::int64_t i64, Context& ctx)
{
    ctx.writer().Int64(i64);
}

template <typename Context>
void dump_adl(std::uint64_t u64, Context& ctx)
{
    ctx.writer().Uint64(u64);
}

template <typename Context>
void dump_adl(double d, Context& ctx)
{
    ctx.writer().Double(d);
}

template <typename Context>
void dump_adl(const typename Context::writer_type::Ch* str, Context& ctx)
{
    ctx.writer().String(str);
}

template <typename Context>
void dump_adl(const std::basic_string<typename Context::writer_type::Ch>& str, Context& ctx)
{
    ctx.writer().String(str);
}

template <typename Context>
void dump_adl(std::basic_string_view<typename Context::writer_type::Ch> str, Context& ctx)
{
    ctx.writer().String(str.data(), static_cast<rapidjson::SizeType>(str.length()));
}

// to_json implementation

// Checks if we can construct a Json object with given T
template <typename T>
using is_json_constructible =
    std::bool_constant<std::is_constructible_v<rapidjson::Value, T> &&
                       // We want reflectable unscoped enum to handle ourselves
                       !std::is_enum_v<T>>;

// For all T's that we can directly create rapidjson::Value value from
template <typename JsonConstructible, typename Context,
          enable_if<is_json_constructible<JsonConstructible>> = true>
rapidjson::Value to_json(const JsonConstructible& value, Context& ctx)
{
    (void)ctx;
    return rapidjson::Value{value};
}

template <typename Context>
rapidjson::Value to_json(view v, Context& ctx)
{
    return rapidjson::Value{v.value(), ctx.allocator()};
}

template <typename Context>
rapidjson::Value to_json(std::nullptr_t, Context&)
{
    return rapidjson::Value{};
}

template <typename Ch, typename Context>
rapidjson::Value to_json(const std::basic_string<Ch>& str, Context& ctx)
{
    return rapidjson::Value{str, ctx.allocator()};
}

template <typename Ch, typename Context>
rapidjson::Value to_json(std::basic_string_view<Ch> str, Context& ctx)
{
    return rapidjson::Value{str.data(), static_cast<rapidjson::SizeType>(str.length()),
                            ctx.allocator()};
}

template <typename Context>
rapidjson::Value to_json(const char* str, Context& ctx)
{
    return rapidjson::Value{str, ctx.allocator()};
}

// For string literals
template <std::size_t N, typename Context>
rapidjson::Value to_json(char (&str)[N], Context&)
{
    // No need for allocator
    return rapidjson::Value{str, N - 1};
}

struct json_tree_backend
{
    using value_type = rapidjson::Value;
    using deserialize_error = json::deserialize_error;

    template <typename T, typename Context>
    static value_type serialize(const T& value, Context& ctx)
    {
        return json::serialize(value, ctx);
    }

    template <typename T>
    static void deserialize(T& out, const value_type& value)
    {
        json::deserialize(out, value);
    }

    // Map stuff

    static value_type make_map() { return value_type{rapidjson::kObjectType}; }
    static void expect_map(const value_type& value) { detail::expect_object(value); }
    static bool is_map(const value_type& value) { return value.IsObject(); }

    template <typename Key, typename Context>
    static void add_field(value_type& out, const Key& key, value_type value, Context& ctx)
    {
        out.AddMember(rapidjson::StringRef(key), std::move(value), ctx.allocator());
    }

    template <typename Visitor>
    static void for_each_field(const value_type& value, Visitor&& visitor)
    {
        for (const auto& obj : value.GetObject())
            visitor(obj.name, obj.value);
    }

    // Sequence stuff

    static value_type make_sequence() { return value_type{rapidjson::kArrayType}; }
    static void expect_sequence(const value_type& value) { detail::expect_array(value); }
    static bool is_sequence(const value_type& value) { return value.IsArray(); }

    template <typename Context>
    static void add_element(value_type& out, value_type value, Context& ctx)
    {
        out.PushBack(std::move(value), ctx.allocator());
    }

    template <typename Visitor>
    static void for_each_element(const value_type& value, Visitor&& visitor)
    {
        for (const auto& item : value.GetArray())
            visitor(item);
    }

    // Rest of the stuff

    static bool is_null(const value_type& value) { return value.IsNull(); }
    static std::size_t size(const value_type& value) { return value.Size(); }

    static const value_type& at_field(const value_type& value, const char* name)
    {
        return json::at(value.GetObject(), name);
    }

    static const value_type& at_index(const value_type& value, std::size_t index)
    {
        return json::at(value.GetArray(), static_cast<rapidjson::SizeType>(index));
    }

    static std::string type_name(const value_type& value) { return detail::type_name(value); }
};

// For all T's that quacks like a std::map
template <typename Map, typename Context,
          enable_if<std::negation<is_json_constructible<Map>>,
                    is_map_alike<Map>> = true>
rapidjson::Value to_json(const Map& map, Context& ctx)
{
    return serialization::detail::serialize_map<json_tree_backend>(map, ctx);
}

// For all T's that quacks like a range
template <typename Range, typename Context,
          enable_if<std::negation<is_json_constructible<Range>>,
                    std::negation<is_map_alike<Range>>, is_range<Range>> = true>
rapidjson::Value to_json(const Range& rng, Context& ctx)
{
    return serialization::detail::serialize_range<json_tree_backend>(rng, ctx);
}

// For all T's for which there's a type_info defined
template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
rapidjson::Value to_json(const Reflectable& refl, Context& ctx)
{
    return serialization::detail::serialize_reflectable<json_tree_backend>(refl, ctx);
}

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
rapidjson::Value to_json(Enum e, Context& ctx)
{
    return serialization::detail::serialize_enum<json_tree_backend>(e, ctx);
}

template <typename Enum, typename Context>
rapidjson::Value to_json(const enum_set<Enum>& set, Context& ctx)
{
    return serialization::detail::serialize_enum_set<json_tree_backend>(set, ctx);
}

template <typename... Ts, typename Context>
rapidjson::Value to_json(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return serialization::detail::serialize_tuple<json_tree_backend>(tuple, ctx);
}

template <typename T, typename Context>
rapidjson::Value to_json(const std::optional<T>& opt, Context& ctx)
{
    return serialization::detail::serialize_optional<json_tree_backend>(opt, ctx);
}

// from_json implementation

[[noreturn]] inline void throw_lossy_conversion()
{
    throw deserialize_error{
        "value cannot be losslessly stored in the variable"};
}

template <typename Target, typename Source>
Target narrow(Source src)
{
    if (static_cast<Source>(static_cast<Target>(src)) != src)
        throw_lossy_conversion();
    return static_cast<Target>(src);
}

template <typename Integral, enable_if<std::is_integral<Integral>> = true>
void from_json(Integral& out, const rapidjson::Value& value)
{
    detail::expect_integral(value);
    // int8, int16 and int32
    if constexpr (std::is_signed_v<Integral> && sizeof(Integral) < 8)
    {
        if (value.IsInt())
        {
            out = narrow<Integral>(value.GetInt());
            return;
        }
    }
    // int64
    else if constexpr (std::is_signed_v<Integral> && sizeof(Integral) == 8)
    {
        if (value.IsInt64())
        {
            out = value.GetInt64();
            return;
        }
    }
    // uint8, uint16 and uint32
    else if constexpr (std::is_unsigned_v<Integral> && sizeof(Integral) < 8)
    {
        if (value.IsUint())
        {
            out = narrow<Integral>(value.GetUint());
            return;
        }
    }
    // uint64
    else if constexpr (std::is_unsigned_v<Integral> && sizeof(Integral) == 8)
    {
        if (value.IsUint64())
        {
            out = value.GetUint64();
            return;
        }
    }
    throw_lossy_conversion();
}

template <typename Floating, enable_if<std::is_floating_point<Floating>> = true>
void from_json(Floating& out, const rapidjson::Value& value)
{
    detail::expect_number(value);
    out = static_cast<Floating>(value.GetDouble());
}

inline void from_json(bool& out, const rapidjson::Value& value)
{
    detail::expect_boolean(value);
    out = value.GetBool();
}

inline void from_json(std::string& out, const rapidjson::Value& value)
{
    detail::expect_string(value);
    out = {value.GetString(),
           static_cast<std::size_t>(value.GetStringLength())};
}

inline void from_json(std::string_view& out, const rapidjson::Value& value)
{
    // This variant is unsafe because the lifetime of underlying string is tied
    // to the lifetime of the JSON's value. It may come in handy when writing
    // user-defined `from_json` which only need a string_view to do further
    // conversion or when one can guarantee `value` will outlive the returned
    // string_view. Nevertheless, use with caution.
    detail::expect_string(value);
    out = {value.GetString(),
           static_cast<std::size_t>(value.GetStringLength())};
}

inline void from_json(view& out, const rapidjson::Value& value)
{
    out = view{value};
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
void from_json(Map& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_map<json_tree_backend>(out, value);
}

template <typename GrowableRange,
          enable_if<std::negation<is_map_alike<GrowableRange>>,
                    is_growable_range<GrowableRange>> = true>
void from_json(GrowableRange& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_range<json_tree_backend>(out, value);
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
void from_json(Reflectable& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_reflectable<json_tree_backend>(out, value);
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
void from_json(Enum& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_enum<json_tree_backend>(out, value);
}

template <typename Enum>
void from_json(enum_set<Enum>& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_enum_set<json_tree_backend>(out, value);
}

template <typename... Ts>
void from_json(std::tuple<Ts...>& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_tuple<json_tree_backend>(out, value);
}

template <typename T>
void from_json(std::optional<T>& out, const rapidjson::Value& value)
{
    serialization::detail::deserialize_optional<json_tree_backend>(out, value);
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
    -> decltype(dump_adl(obj, ctx), void())
{
    dump_adl(obj, ctx);
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(json::serializer<T>::dump(obj, ctx), void())
{
    json::serializer<T>::dump(obj, ctx);
}

template <typename T, typename Context>
rapidjson::Value serialize(const T&, Context&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot serialize an instance of type T - no viable "
                  "definition of to_json provided");
    return {}; // Keeps compiler happy
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<1>)
    -> decltype(to_json(obj, ctx))
{
    return to_json(obj, ctx);
}

template <typename T, typename Context>
auto serialize(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(json::serializer<T>::to_json(obj, ctx))
{
    return json::serializer<T>::to_json(obj, ctx);
}

template <typename T>
void deserialize(T&, const rapidjson::Value&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of from_json provided");
}

template <typename T>
auto deserialize(T& out, const rapidjson::Value& value, priority_tag<1>)
    -> decltype(from_json(out, value), void())
{
    from_json(out, value);
}

template <typename T>
auto deserialize(T& out, const rapidjson::Value& value, priority_tag<2>)
    -> decltype(json::serializer<T>::from_json(out, value), void())
{
    json::serializer<T>::from_json(out, value);
}
} // namespace detail

template <typename T>
std::string dump(const T& obj)
{
    using namespace rapidjson;
    StringBuffer buf{};
    Writer<StringBuffer> wrt{buf};
    dump_context<Writer<StringBuffer>> ctx{wrt};

    json::dump(obj, ctx);
    return {buf.GetString()};
}

template <typename T, typename Context>
void dump(const T& obj, Context& ctx)
{
    detail::dump(obj, ctx, priority_tag<2>{});
}

// Top-level functions
template <typename T>
rapidjson::Document serialize(const T& obj)
{
    rapidjson::Document doc;
    rapidjson::Value& v = doc;
    serialize_context ctx{doc};
    v = json::serialize(obj, ctx);
    return doc;
}

template <typename T, typename Context>
rapidjson::Value serialize(const T& obj, Context& ctx)
{
    return detail::serialize(obj, ctx, priority_tag<2>{});
}

template <typename T>
void deserialize(T& out, const rapidjson::Value& value)
{
    detail::deserialize(out, value, priority_tag<2>{});
}

// Shorter version of from which can't be overloaded. Only use to invoke
// the from() without providing a bit weird first parameter.
template <typename T>
T deserialize(const rapidjson::Value& value)
{
    static_assert(std::is_default_constructible_v<T>,
                  "T must be default constructible");
    T out;
    json::deserialize(out, value);
    return out;
}
} // namespace kl::json

inline rapidjson::Document operator""_json(const char* s, std::size_t len)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s, len);
    if (!ok)
        throw kl::json::parse_error{rapidjson::GetParseError_En(ok.Code())};
    return doc;
}
