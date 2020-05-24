#pragma once

#include "kl/type_traits.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_set.hpp"
#include "kl/tuple.hpp"
#include "kl/utility.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <optional>
#include <exception>
#include <string>
#include <string_view>

#if defined(GetObject)
#undef GetObject
#endif

namespace rapidjson {

KL_DESCRIBE_ENUM(Type, kNullType, kFalseType, kTrueType, kObjectType,
                 kArrayType, kStringType, kNumberType)
} // namespace rapidjson

namespace kl {
namespace json {

template <typename T>
struct encoder;

template <typename T>
bool is_null_value(const T&) { return false; }

template <typename T>
bool is_null_value(const std::optional<T>& opt) { return !opt; }

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

template <typename T>
std::string dump(const T& obj);

template <typename T, typename Context>
void dump(const T& obj, Context& ctx);

template <typename T>
struct serializer;

class serialize_context
{
public:
    using allocator_type = rapidjson::Document::AllocatorType;

    explicit serialize_context(allocator_type& allocator,
                               bool skip_null_fields = true)
        : allocator_{allocator}, skip_null_fields_{skip_null_fields}
    {
    }

    rapidjson::MemoryPoolAllocator<>& allocator() { return allocator_; }

    template <typename Key, typename Value>
    bool skip_field(const Key&, const Value& value)
    {
        return skip_null_fields_ && is_null_value(value);
    }

private:
    allocator_type& allocator_;
    bool skip_null_fields_;
};

template <typename T>
rapidjson::Document serialize(const T& obj);

template <typename T, typename Context>
rapidjson::Value serialize(const T& obj, Context& ctx);

template <typename T>
T deserialize(type_t<T>, const rapidjson::Value& value);

// Shorter version of from which can't be overloaded. Only use to invoke
// the from() without providing a bit weird first parameter.
template <typename T>
T deserialize(const rapidjson::Value& value)
{
    return json::deserialize(type<T>, value);
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
        has_iterator<T>> {};

// encode implementation

template <typename Context>
void encode(std::nullptr_t, Context& ctx)
{
    ctx.writer().Null();
}

template <typename Context>
void encode(bool b, Context& ctx)
{
    ctx.writer().Bool(b);
}

template <typename Context>
void encode(int i, Context& ctx)
{
    ctx.writer().Int(i);
}

template <typename Context>
void encode(unsigned int u, Context& ctx)
{
    ctx.writer().Uint(u);
}

template <typename Context>
void encode(std::int64_t i64, Context& ctx)
{
    ctx.writer().Int64(i64);
}

template <typename Context>
void encode(std::uint64_t u64, Context& ctx)
{
    ctx.writer().Uint64(u64);
}

template <typename Context>
void encode(double d, Context& ctx)
{
    ctx.writer().Double(d);
}

template <typename Context>
void encode(const typename Context::writer_type::Ch* str, Context& ctx)
{
    ctx.writer().String(str);
}

template <typename Context>
void encode(const std::basic_string<typename Context::writer_type::Ch>& str,
            Context& ctx)
{
    ctx.writer().String(str);
}

template <typename Context>
void encode(std::basic_string_view<typename Context::writer_type::Ch> str,
            Context& ctx)
{
    ctx.writer().String(str.data(),
                        static_cast<rapidjson::SizeType>(str.length()));
}

template <typename Key, typename Context>
void encode_key(const Key& key, Context& ctx)
{
    ctx.writer().Key(key.c_str(), static_cast<rapidjson::SizeType>(key.size()));
}

template <typename Context>
void encode_key(const char* key, Context& ctx)
{
    ctx.writer().Key(key);
}

template <typename Map, typename Context, enable_if<is_map_alike<Map>> = true>
void encode(const Map& map, Context& ctx)
{
    ctx.writer().StartObject();
    for (const auto& kv : map)
    {
        if (!ctx.skip_field(kv.first, kv.second))
        {
            encode_key(kv.first, ctx);
            json::dump(kv.second, ctx);
        }
    }
    ctx.writer().EndObject();
}

template <
    typename Vector, typename Context,
    enable_if<negation<is_map_alike<Vector>>, is_vector_alike<Vector>> = true>
void encode(const Vector& vec, Context& ctx)
{
    ctx.writer().StartArray();
    for (const auto& v : vec)
        json::dump(v, ctx);
    ctx.writer().EndArray();
}

template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
void encode(const Reflectable& refl, Context& ctx)
{
    ctx.writer().StartObject();
    ctti::reflect(refl, [&ctx](auto fi) {
        if (!ctx.skip_field(fi.name(), fi.get()))
        {
            ctx.writer().Key(fi.name());
            json::dump(fi.get(), ctx);
        }
    });
    ctx.writer().EndObject();
}

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
void encode(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        json::dump(kl::to_string(e), ctx);
    else
        json::dump(underlying_cast(e), ctx);
}

template <typename Enum, typename Context>
void encode(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");
    ctx.writer().StartArray();
    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (set.test(possible_value))
            json::dump(kl::to_string(possible_value), ctx);
    }
    ctx.writer().EndArray();
}

template <typename Tuple, std::size_t... Is, typename Context>
void encode_tuple(const Tuple& tuple, Context& ctx, index_sequence<Is...>)
{
    ctx.writer().StartArray();
    (json::dump(std::get<Is>(tuple), ctx), ...);
    ctx.writer().EndArray();
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
        ctx.writer().Null();
    else
        json::dump(*opt, ctx);
}

// to_json implementation

// Checks if we can construct a Json object with given T
template <typename T>
using is_json_constructible =
    bool_constant<std::is_constructible_v<rapidjson::Value, T> &&
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

template <typename Ch, typename Context>
rapidjson::Value to_json(const std::basic_string<Ch>& str, Context& ctx)
{
    return rapidjson::Value{str, ctx.allocator()};
}

template <typename Ch, typename Context>
rapidjson::Value to_json(std::basic_string_view<Ch> str, Context& ctx)
{
    return rapidjson::Value{str.data(),
                            static_cast<rapidjson::SizeType>(str.length()),
                            ctx.allocator()};
}

template <typename Context>
rapidjson::Value to_json(const char* str, Context& ctx)
{
    return rapidjson::Value{str, ctx.allocator()};
}

// For string literals
template <std::size_t N, typename Context>
rapidjson::Value to_json(char (&str)[N], Context& ctx)
{
    // No need for allocator
    return rapidjson::Value{str, N - 1};
}

// For all T's that quacks like a std::map
template <
    typename Map, typename Context,
    enable_if<negation<is_json_constructible<Map>>, is_map_alike<Map>> = true>
rapidjson::Value to_json(const Map& map, Context& ctx)
{
    static_assert(std::is_constructible_v<std::string, typename Map::key_type>,
                  "std::string must be constructible from the Map's key type");

    rapidjson::Value obj{rapidjson::kObjectType};
    for (const auto& kv : map)
    {
        if (!ctx.skip_field(kv.first, kv.second))
        {
            obj.AddMember(rapidjson::StringRef(kv.first),
                          json::serialize(kv.second, ctx), ctx.allocator());
        }
    }
    return obj;
}

// For all T's that quacks like a std::vector
template <
    typename Vector, typename Context,
    enable_if<negation<is_json_constructible<Vector>>,
              negation<is_map_alike<Vector>>, is_vector_alike<Vector>> = true>
rapidjson::Value to_json(const Vector& vec, Context& ctx)
{
    rapidjson::Value arr{rapidjson::kArrayType};
    for (const auto& v : vec)
        arr.PushBack(json::serialize(v, ctx), ctx.allocator());
    return arr;
}

// For all T's for which there's a type_info defined
template <typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
rapidjson::Value to_json(const Reflectable& refl, Context& ctx)
{
    rapidjson::Value obj{rapidjson::kObjectType};
    ctti::reflect(refl, [&obj, &ctx](auto fi) {
        if (!ctx.skip_field(fi.name(), fi.get()))
        {
            auto json = json::serialize(fi.get(), ctx);
            obj.AddMember(rapidjson::StringRef(fi.name()), std::move(json),
                          ctx.allocator());
        }
    });
    return obj;
}

template <typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
rapidjson::Value to_json(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        return rapidjson::Value{rapidjson::StringRef(kl::to_string(e)),
                                ctx.allocator()};
    else
        return to_json(underlying_cast(e), ctx);
}

template <typename Enum, typename Context>
rapidjson::Value to_json(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");
    rapidjson::Value arr{rapidjson::kArrayType};

    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (set.test(possible_value))
            arr.PushBack(to_json(possible_value, ctx), ctx.allocator());
    }

    return arr;
}

template <typename Tuple, typename Context, std::size_t... Is>
rapidjson::Value tuple_to_json(const Tuple& tuple, Context& ctx,
                               index_sequence<Is...>)
{
    rapidjson::Value arr{rapidjson::kArrayType};
    (arr.PushBack(json::serialize(std::get<Is>(tuple), ctx), ctx.allocator()),
     ...);
    return arr;
}

template <typename... Ts, typename Context>
rapidjson::Value to_json(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return tuple_to_json(tuple, ctx, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T, typename Context>
rapidjson::Value to_json(const std::optional<T>& opt, Context& ctx)
{
    if (opt)
        return json::serialize(*opt, ctx);
    return rapidjson::Value{};
}

// from_json implementation

inline std::string json_type_name(const rapidjson::Value& value)
{
    return kl::to_string(value.GetType());
}

template <typename T>
struct is_64bit : kl::bool_constant<sizeof(T) == 8> {};

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

class integral_value_extractor
{
public:
    explicit integral_value_extractor(const rapidjson::Value& value)
        : value_{value}
    {
    }

    // int8, int16 and int32
    template <typename T,
              enable_if<std::is_signed<T>, negation<is_64bit<T>>> = true>
    explicit operator T() const
    {
        if (!value_.IsInt())
        {
            if (value_.IsInt64() || value_.IsUint64())
                // Value to big but it is still an integral
                throw_lossy_conversion();

            throw deserialize_error{"type must be an integral but is a " +
                                    json_type_name(value_)};
        }
        return narrow<T>(value_.GetInt());
    }

    // int64
    // It's a template operator instead of a just int64_t operator because long
    // and long long are distinct types on Linux (LP64)
    template <typename T, enable_if<std::is_signed<T>, is_64bit<T>> = true>
    explicit operator T() const
    {
        if (!value_.IsInt64())
        {
            if (value_.IsUint64())
                // Value to big but it is still an integral
                throw_lossy_conversion();

            throw deserialize_error{"type must be an integral but is a " +
                                    json_type_name(value_)};
        }
        return value_.GetInt64();
    }

    // uint8, uint16 and uint32
    template <typename T,
              enable_if<std::is_unsigned<T>, negation<is_64bit<T>>> = true>
    explicit operator T() const
    {
        if (!value_.IsUint())
        {
            if (value_.IsUint64() || value_.IsInt64())
                throw_lossy_conversion();

            throw deserialize_error{"type must be an integral but is a " +
                                    json_type_name(value_)};
        }
        return narrow<T>(value_.GetUint());
    }

    // uint64
    template <typename T, enable_if<std::is_unsigned<T>, is_64bit<T>> = true>
    explicit operator T() const
    {
        if (!value_.IsUint64())
        {
            if (value_.IsInt64())
                throw_lossy_conversion();

            throw deserialize_error{"type must be an integral but is a " +
                                    json_type_name(value_)};
        }
        return value_.GetUint64();
    }

private:
    const rapidjson::Value& value_;
};

template <typename Integral, enable_if<std::is_integral<Integral>> = true>
Integral from_json(type_t<Integral>, const rapidjson::Value& value)
{
    return static_cast<Integral>(integral_value_extractor{value});
}

template <typename Floating, enable_if<std::is_floating_point<Floating>> = true>
Floating from_json(type_t<Floating>, const rapidjson::Value& value)
{
    if (!value.IsNumber())
        throw deserialize_error{"type must be a floating-point but is a " +
                                json_type_name(value)};

    return static_cast<Floating>(value.GetDouble());
}

inline bool from_json(type_t<bool>, const rapidjson::Value& value)
{
    if (!value.IsBool())
        throw deserialize_error{"type must be a bool but is a " +
                                json_type_name(value)};
    return value.GetBool();
}

inline std::string from_json(type_t<std::string>, const rapidjson::Value& value)
{
    if (!value.IsString())
        throw deserialize_error{"type must be a string but is a " +
                                json_type_name(value)};

    return {value.GetString(),
            static_cast<std::size_t>(value.GetStringLength())};
}

inline std::string_view from_json(type_t<std::string_view>,
                                  const rapidjson::Value& value)
{
    // This variant is unsafe because the lifetime of underlying string is tied
    // to the lifetime of the JSON's value. It may come in handy when writing
    // user-defined `from_json` which only need a string_view to do further
    // conversion or when one can guarantee `value` will outlive the returned
    // string_view. Nevertheless, use with caution.
    if (!value.IsString())
        throw deserialize_error{"type must be a string but is a " +
                                json_type_name(value)};

    return {value.GetString(),
            static_cast<std::size_t>(value.GetStringLength())};
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
Map from_json(type_t<Map>, const rapidjson::Value& value)
{
    if (!value.IsObject())
        throw deserialize_error{"type must be an object but is a " +
                                json_type_name(value)};

    Map ret{};

    for (const auto& obj : value.GetObject())
    {
        try
        {
            ret.emplace(
                json::deserialize<typename Map::key_type>(obj.name),
                json::deserialize<typename Map::mapped_type>(obj.value));
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing field " +
                              json::deserialize<std::string>(obj.name);
            ex.add(msg.c_str());
            throw;
        }
    }

    return ret;
}

template <typename Vector, enable_if<negation<is_map_alike<Vector>>,
                                     is_vector_alike<Vector>> = true>
Vector from_json(type_t<Vector>, const rapidjson::Value& value)
{
    if (!value.IsArray())
        throw deserialize_error{"type must be an array but is a " +
                                json_type_name(value)};

    Vector ret{};
    if constexpr (has_reserve_v<Vector>)
        ret.reserve(value.Size());

    for (const auto& item : value.GetArray())
    {
        try
        {
            ret.push_back(json::deserialize<typename Vector::value_type>(item));
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

// Safely gets the JSON from the array of JSON values. If provided index is
// out-of-bounds we return a null value.
inline const rapidjson::Value&
    safe_get_value(const rapidjson::Value::ConstArray& arr,
                   rapidjson::SizeType idx)
{
    static const auto null_value = rapidjson::Value{};
    return idx >= arr.Size() ? null_value : arr[idx];
}

template <typename Reflectable>
Reflectable reflectable_from_json(const rapidjson::Value& value)
{
    Reflectable refl{};

    if (value.IsObject())
    {
        const auto obj = value.GetObject();
        ctti::reflect(refl, [&obj](auto fi) {
            using field_type = typename decltype(fi)::type;

            try
            {
                const auto it = obj.FindMember(fi.name());
                if (it != obj.end())
                    fi.get() = json::deserialize<field_type>(it->value);
                else
                    fi.get() = json::deserialize<field_type>({});
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
    else if (value.IsArray())
    {
        if (value.Size() > ctti::total_num_fields<Reflectable>())
        {
            throw deserialize_error{"array size is greater than "
                                    "declared struct's field "
                                    "count"};
        }
        const auto arr = value.GetArray();
        ctti::reflect(refl, [&arr, index = 0U](auto fi) mutable {
            using field_type = typename decltype(fi)::type;

            try
            {
                const auto& j = safe_get_value(arr, index);
                fi.get() = json::deserialize<field_type>(j);
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
        throw deserialize_error{"type must be an array or object but is a " +
                                json_type_name(value)};
    }

    return refl;
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
Reflectable from_json(type_t<Reflectable>, const rapidjson::Value& value)
{
    static_assert(std::is_default_constructible_v<Reflectable>,
                  "Reflectable must be default constructible");

    try
    {
        return reflectable_from_json<Reflectable>(value);
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
Enum from_json(type_t<Enum>, const rapidjson::Value& value)
{
    if constexpr (is_enum_reflectable_v<Enum>)
    {
        if (!value.IsString())
            throw deserialize_error{"type must be a string-enum but is a " +
                                    json_type_name(value)};
        if (auto enum_value = kl::from_string<Enum>(
                std::string_view{value.GetString(), value.GetStringLength()}))
        {
            return *enum_value;
        }
        throw deserialize_error{"invalid enum value: " +
                                json::deserialize<std::string>(value)};
    }
    else
    {
        if (!value.IsNumber())
            throw deserialize_error{"type must be a number-enum but is a " +
                                    json_type_name(value)};
        return static_cast<Enum>(value.GetInt());
    }
}

template <typename Enum>
enum_set<Enum> from_json(type_t<enum_set<Enum>>, const rapidjson::Value& value)
{
    if (!value.IsArray())
        throw deserialize_error{"type must be an array but is a " +
                                json_type_name(value)};

    enum_set<Enum> ret{};

    for (const auto& v : value.GetArray())
    {
        const auto e = json::deserialize<Enum>(v);
        ret |= e;
    }

    return ret;
}

template <typename Tuple, std::size_t... Is>
static Tuple tuple_from_json(const rapidjson::Value& value,
                             index_sequence<Is...>)
{
    const auto arr = value.GetArray();
    return std::make_tuple(json::deserialize<std::tuple_element_t<Is, Tuple>>(
        safe_get_value(arr, Is))...);
}

template <typename... Ts>
std::tuple<Ts...> from_json(type_t<std::tuple<Ts...>>,
                            const rapidjson::Value& value)
{
    if (!value.IsArray())
        throw deserialize_error{"type must be an array but is a " +
                                json_type_name(value)};

    return tuple_from_json<std::tuple<Ts...>>(
        value, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
std::optional<T> from_json(type_t<std::optional<T>>,
                           const rapidjson::Value& value)
{
    if (value.IsNull())
        return std::nullopt;
    return json::deserialize<T>(value);
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
    -> decltype(encode(obj, ctx), void())
{
    encode(obj, ctx);
}

template <typename T, typename Context>
auto dump(const T& obj, Context& ctx, priority_tag<2>)
    -> decltype(json::encoder<T>::encode(obj, ctx), void())
{
    json::encoder<T>::encode(obj, ctx);
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
T deserialize(const rapidjson::Value&, priority_tag<0>)
{
    static_assert(always_false_v<T>,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of from_json provided");
    return T{}; // Keeps compiler happy
}

template <typename T>
auto deserialize(const rapidjson::Value& value, priority_tag<1>)
    -> decltype(from_json(type<T>, value))
{
    return from_json(type<T>, value);
}

template <typename T>
auto deserialize(const rapidjson::Value& value, priority_tag<2>)
    -> decltype(json::serializer<T>::from_json(value))
{
    return json::serializer<T>::from_json(value);
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
    serialize_context ctx{doc.GetAllocator()};
    v = json::serialize(obj, ctx);
    return doc;
}

template <typename T, typename Context>
rapidjson::Value serialize(const T& obj, Context& ctx)
{
    return detail::serialize(obj, ctx, priority_tag<2>{});
}

template <typename T>
T deserialize(type_t<T>, const rapidjson::Value& value)
{
    return detail::deserialize<T>(value, priority_tag<2>{});
}
} // namespace json
} // namespace kl

inline rapidjson::Document operator""_json(const char* s, std::size_t len)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s, len);
    if (!ok)
        throw kl::json::parse_error{rapidjson::GetParseError_En(ok.Code())};
    return doc;
}
