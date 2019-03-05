#pragma once

#include "kl/type_traits.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_flags.hpp"
#include "kl/tuple.hpp"
#include "kl/utility.hpp"

#include <boost/optional.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <exception>
#include <string>

KL_DEFINE_ENUM_REFLECTOR(rapidjson, Type,
                         (kNullType, kFalseType, kTrueType, kObjectType,
                          kArrayType, kStringType, kNumberType))

namespace kl {
namespace json {

template <typename T>
struct encoder;

template <typename T>
std::string dump(const T& obj);

template <typename T, typename Writer>
void dump(const T& obj, Writer& writer);

template <typename T>
struct serializer;

template <typename T>
rapidjson::Document serialize(const T& obj);

template <typename T>
rapidjson::Value serialize(const T& obj, rapidjson::Document& doc);

template <typename T>
T deserialize(type_t<T>, const rapidjson::Value& value);

// Shorter version of from which can't be overloaded. Only use to invoke
// the from() without providing a bit weird first parameter.
template <typename T>
T deserialize(const rapidjson::Value& value)
{
    // TODO replace with variable template type<T>
    return json::deserialize(type_t<T>{}, value);
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

template <typename Writer>
void encode(std::nullptr_t, Writer& writer)
{
    writer.Null();
}

template <typename Writer>
void encode(bool b, Writer& writer)
{
    writer.Bool(b);
}

template <typename Writer>
void encode(int i, Writer& writer)
{
    writer.Int(i);
}

template <typename Writer>
void encode(unsigned int u, Writer& writer)
{
    writer.Uint(u);
}

template <typename Writer>
void encode(std::int64_t i64, Writer& writer)
{
    writer.Int64(i64);
}

template <typename Writer>
void encode(std::uint64_t u64, Writer& writer)
{
    writer.Uint64(u64);
}

template <typename Writer>
void encode(double d, Writer& writer)
{
    writer.Double(d);
}

template <typename Writer>
void encode(const typename Writer::Ch* str, Writer& writer)
{
    writer.String(str);
}

template <typename Writer>
void encode(const std::basic_string<typename Writer::Ch>& str, Writer& writer)
{
    writer.String(str);
}

template <typename Key, typename Writer>
void encode_key(const Key& key, Writer& writer)
{
    writer.Key(key.c_str(), key.size());
}

template <typename Writer>
void encode_key(const char* key, Writer& writer)
{
    writer.Key(key);
}

template <typename Map, typename Writer, enable_if<is_map_alike<Map>> = true>
void encode(const Map& map, Writer& writer)
{
    writer.StartObject();
    for (const auto& kv : map)
    {
        encode_key(kv.first, writer);
        json::dump(kv.second, writer);
    }
    writer.EndObject();
}

template <
    typename Vector, typename Writer,
    enable_if<negation<is_map_alike<Vector>>, is_vector_alike<Vector>> = true>
void encode(const Vector& vec, Writer& writer)
{
    writer.StartArray();
    for (const auto& v : vec)
        json::dump(v, writer);
    writer.EndArray();
}

template <typename Reflectable, typename Writer,
          enable_if<is_reflectable<Reflectable>> = true>
void encode(const Reflectable& refl, Writer& writer)
{
    writer.StartObject();
    ctti::reflect(refl, [&writer](auto fi) {
        writer.Key(fi.name());
        json::dump(fi.get(), writer);
    });
    writer.EndObject();
}

template <typename Enum, typename Writer>
void encode_enum(Enum e, Writer& writer,
                 std::false_type /*is_enum_reflectable*/)
{
    json::dump(underlying_cast(e), writer);
}

template <typename Enum, typename Writer>
void encode_enum(Enum e, Writer& writer, std::true_type /*is_enum_reflectable*/)
{
    json::dump(enum_reflector<Enum>::to_string(e), writer);
}

template <typename Enum, typename Writer, enable_if<std::is_enum<Enum>> = true>
void encode(Enum e, Writer& writer)
{
    encode_enum(e, writer, is_enum_reflectable<Enum>{});
}

template <typename Enum, typename Writer>
void encode(const enum_flags<Enum>& flags, Writer& writer)
{
    static_assert(is_enum_reflectable<Enum>::value,
                  "Only flags of reflectable enums are supported");
    writer.StartArray();
    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (flags.test(possible_value))
            json::dump(enum_reflector<Enum>::to_string(possible_value), writer);
    }
    writer.EndArray();
}

template <typename Tuple, std::size_t... Is, typename Writer>
void encode_tuple(const Tuple& tuple, Writer& writer, index_sequence<Is...>)
{
    writer.StartArray();
    using swallow = std::initializer_list<int>;
    (void)swallow{((json::dump(std::get<Is>(tuple), writer)), 0)...};
    writer.EndArray();
}

template <typename... Ts, typename Writer>
void encode(const std::tuple<Ts...>& tuple, Writer& writer)
{
    encode_tuple(tuple, writer, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T, typename Writer>
void encode(const boost::optional<T>& opt, Writer& writer)
{
    if (!opt)
        writer.Null();
    else
        json::dump(*opt, writer);
}

// Checks if we can construct a Json object with given T
template <typename T>
using is_json_constructible =
    bool_constant<std::is_constructible<rapidjson::Value, T>::value &&
                  // We want reflectable unscoped enum to handle ourselves
                  !std::is_enum<T>::value>;

KL_VALID_EXPR_HELPER(has_reserve, std::declval<T&>().reserve(0U))

// For all T's that we can directly create rapidjson::Value value from
template <typename JsonConstructible,
          enable_if<is_json_constructible<JsonConstructible>> = true>
rapidjson::Value to_json(const JsonConstructible& value, rapidjson::Document&)
{
    return rapidjson::Value{value};
}

template <typename Ch>
rapidjson::Value to_json(const std::basic_string<Ch>& str,
                         rapidjson::Document& doc)
{
    return rapidjson::Value{str, doc.GetAllocator()};
}

// For string literals
template <std::size_t N>
rapidjson::Value to_json(const char (&str)[N], rapidjson::Document&)
{
    // No need for allocator
    return rapidjson::Value{str, N};
}

// For all T's that quacks like a std::map
template <typename Map, enable_if<negation<is_json_constructible<Map>>,
                                  is_map_alike<Map>> = true>
rapidjson::Value to_json(const Map& map, rapidjson::Document& doc)
{
    static_assert(
        std::is_constructible<std::string, typename Map::key_type>::value,
        "std::string must be constructible from the Map's key type");

    rapidjson::Value obj(rapidjson::kObjectType);
    for (const auto& kv : map)
        obj.AddMember(rapidjson::StringRef(kv.first),
                      json::serialize(kv.second, doc), doc.GetAllocator());
    return obj;
}

// For all T's that quacks like a std::vector
template <typename Vector, enable_if<negation<is_json_constructible<Vector>>,
                                     negation<is_map_alike<Vector>>,
                                     is_vector_alike<Vector>> = true>
rapidjson::Value to_json(const Vector& vec, rapidjson::Document& doc)
{
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto& v : vec)
        arr.PushBack(json::serialize(v, doc), doc.GetAllocator());
    return arr;
}

// For all T's for which there's a type_info defined
template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
rapidjson::Value to_json(const Reflectable& refl, rapidjson::Document& doc)
{
    rapidjson::Value obj{rapidjson::kObjectType};
    ctti::reflect(refl, [&obj, &doc](auto fi) {
        auto json = json::serialize(fi.get(), doc);
        if (!json.IsNull())
            obj.AddMember(rapidjson::StringRef(fi.name()), std::move(json),
                          doc.GetAllocator());
    });
    return obj;
}

template <typename Enum>
rapidjson::Value enum_to_json(Enum e, rapidjson::Document& doc,
                              std::true_type /*is_enum_reflectable*/)
{
    return rapidjson::Value{
        rapidjson::StringRef(enum_reflector<Enum>::to_string(e)),
        doc.GetAllocator()};
}

template <typename Enum>
rapidjson::Value enum_to_json(Enum e, rapidjson::Document& doc,
                              std::false_type /*is_enum_reflectable*/)
{
    return json::serialize(underlying_cast(e), doc);
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
rapidjson::Value to_json(Enum e, rapidjson::Document& doc)
{
    return enum_to_json(e, doc, is_enum_reflectable<Enum>{});
}

template <typename Enum>
rapidjson::Value to_json(const enum_flags<Enum>& flags,
                         rapidjson::Document& doc)
{
    static_assert(is_enum_reflectable<Enum>::value,
                  "Only flags of reflectable enums are supported");
    rapidjson::Value arr(rapidjson::kArrayType);

    for (const auto possible_value : enum_reflector<Enum>::values())
    {
        if (flags.test(possible_value))
            arr.PushBack(to_json(possible_value, doc), doc.GetAllocator());
    }

    return arr;
}

template <typename Tuple, std::size_t... Is>
rapidjson::Value tuple_to_json(const Tuple& tuple, rapidjson::Document& doc,
                               index_sequence<Is...>)
{
    rapidjson::Value arr{rapidjson::kArrayType};
    using swallow = std::initializer_list<int>;
    (void)swallow{(arr.PushBack(json::serialize(std::get<Is>(tuple), doc),
                                doc.GetAllocator()),
                   0)...};
    return arr;
}

template <typename... Ts>
rapidjson::Value to_json(const std::tuple<Ts...>& tuple,
                         rapidjson::Document& doc)
{
    return tuple_to_json(tuple, doc, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
rapidjson::Value to_json(const boost::optional<T>& opt,
                         rapidjson::Document& doc)
{
    if (opt)
        return json::serialize(*opt, doc);
    return rapidjson::Value{};
}

// from_json implementation

inline std::string json_type_name(const rapidjson::Value& value)
{
    return {enum_reflector<rapidjson::Type>::to_string(value.GetType())};
}

template <typename T>
struct is_64bit : kl::bool_constant<sizeof(T) == 8> {};

class arithmetic_value_extractor
{
public:
    explicit arithmetic_value_extractor(const rapidjson::Value& value)
        : value_{value}
    {
    }

    template <typename T,
              enable_if<std::is_signed<T>, negation<is_64bit<T>>> = true>
    operator T() const
    {
        return static_cast<T>(value_.GetInt());
    }

    template <typename T, enable_if<std::is_signed<T>, is_64bit<T>> = true>
    operator T() const
    {
        return value_.GetInt64();
    }

    template <typename T,
              enable_if<std::is_unsigned<T>, negation<is_64bit<T>>> = true>
    operator T() const
    {
        return static_cast<T>(value_.GetUint());
    }

    template <typename T, enable_if<std::is_unsigned<T>, is_64bit<T>> = true>
    operator T() const
    {
        return value_.GetUint64();
    }

    operator float() const { return value_.GetFloat(); }
    operator double() const { return value_.GetDouble(); }

private:
    const rapidjson::Value& value_;
};

template <typename Integral, enable_if<std::is_integral<Integral>> = true>
Integral from_json(type_t<Integral>, const rapidjson::Value& value)
{
    if (!value.IsNumber())
        throw deserialize_error{"type must be an integral but is " +
                                json_type_name(value)};

    return static_cast<Integral>(arithmetic_value_extractor{value});
}

template <typename Floating, enable_if<std::is_floating_point<Floating>> = true>
Floating from_json(type_t<Floating>, const rapidjson::Value& value)
{
    if (!value.IsNumber())
        throw deserialize_error{"type must be a floating-point but is " +
                                json_type_name(value)};

    return static_cast<Floating>(arithmetic_value_extractor{value});
}

inline bool from_json(type_t<bool>, const rapidjson::Value& value)
{
    if (!value.IsBool())
        throw deserialize_error{"type must be a bool but is " +
                                json_type_name(value)};
    return value.GetBool();
}

inline std::string from_json(type_t<std::string>, const rapidjson::Value& value)
{
    if (!value.IsString())
        throw deserialize_error{"type must be a string but is " +
                                json_type_name(value)};

    return {value.GetString(),
            static_cast<std::size_t>(value.GetStringLength())};
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
Map from_json(type_t<Map>, const rapidjson::Value& value)
{
    if (!value.IsObject())
        throw deserialize_error{"type must be an object but is " +
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
Vector from_json(type_t<Vector>, const rapidjson::Value& value)
{
    if (!value.IsArray())
        throw deserialize_error{"type must be an array but is " +
                                json_type_name(value)};

    Vector ret{};
    vector_reserve(ret, value.Size(), has_reserve<Vector>{});

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
        throw deserialize_error{"type must be an array or object but is " +
                                json_type_name(value)};
    }

    return refl;
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
Reflectable from_json(type_t<Reflectable>, const rapidjson::Value& value)
{
    static_assert(std::is_default_constructible<Reflectable>::value,
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
        throw deserialize_error{"type must be an array but is " +
                                json_type_name(value)};

    return tuple_from_json<std::tuple<Ts...>>(
        value, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
boost::optional<T> from_json(type_t<boost::optional<T>>,
                             const rapidjson::Value& value)
{
    if (value.IsNull())
        return {};
    return json::deserialize<T>(value);
}

template <typename Enum>
Enum enum_from_json(const rapidjson::Value& value,
                    std::false_type /*is_enum_reflectable*/)
{
    if (!value.IsNumber())
        throw deserialize_error{"type must be a number-enum but is " +
                                json_type_name(value)};
    return static_cast<Enum>(value.GetInt());
}

template <typename Enum>
Enum enum_from_json(const rapidjson::Value& value,
                    std::true_type /*is_enum_reflectable*/)
{
    if (!value.IsString())
        throw deserialize_error{"type must be a string-enum but is " +
                                json_type_name(value)};
    if (auto enum_value = enum_reflector<Enum>::from_string(
            gsl::cstring_span<>(value.GetString(), value.GetStringLength())))
    {
        return enum_value.get();
    }
    throw deserialize_error{"invalid enum value: " +
                            json::deserialize<std::string>(value)};
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
Enum from_json(type_t<Enum>, const rapidjson::Value& value)
{
    return enum_from_json<Enum>(value, is_enum_reflectable<Enum>{});
}

template <typename Enum>
enum_flags<Enum> from_json(type_t<enum_flags<Enum>>,
                           const rapidjson::Value& value)
{
    if (!value.IsArray())
        throw deserialize_error{"type must be an array but is " +
                                json_type_name(value)};

    enum_flags<Enum> ret{};

    for (const auto& v : value.GetArray())
    {
        const auto e = json::deserialize<Enum>(v);
        ret |= e;
    }

    return ret;
}

template <typename T, typename Writer>
void dump(const T&, Writer&, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot dump an instance of type T - no viable "
                  "definition of encode provided");
}

template <typename T, typename Writer>
auto dump(const T& obj, Writer& writer, priority_tag<1>)
    -> decltype(encode(obj, writer), void())
{
    encode(obj, writer);
}

template <typename T, typename Writer>
auto dump(const T& obj, Writer& writer, priority_tag<2>)
    -> decltype(json::encoder<T>::encode(obj, writer), void())
{
    json::encoder<T>::encode(obj, writer);
}

template <typename T>
rapidjson::Value serialize(const T&, rapidjson::Document&, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot serialize an instance of type T - no viable "
                  "definition of to_json provided");
    return {}; // Keeps compiler happy
}

template <typename T>
auto serialize(const T& obj, rapidjson::Document& doc, priority_tag<1>)
    -> decltype(to_json(obj, doc))
{
    return to_json(obj, doc);
}

template <typename T>
auto serialize(const T& obj, rapidjson::Document& doc, priority_tag<2>)
    -> decltype(json::serializer<T>::to_json(obj, doc))
{
    return json::serializer<T>::to_json(obj, doc);
}

template <typename T>
T deserialize(const rapidjson::Value&, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of from_json provided");
    return T{}; // Keeps compiler happy
}

template <typename T>
auto deserialize(const rapidjson::Value& value, priority_tag<1>)
    -> decltype(from_json(std::declval<type_t<T>>(), value))
{
    // TODO replace with variable template type<T>
    return from_json(type_t<T>{}, value);
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
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer{sb};
    json::dump(obj, writer);
    return {sb.GetString()};
}

template <typename T, typename Writer>
void dump(const T& obj, Writer& writer)
{
    detail::dump(obj, writer, priority_tag<2>{});
}

// Top-level functions
template <typename T>
rapidjson::Document serialize(const T& obj)
{
    rapidjson::Document doc;
    rapidjson::Value& v = doc;
    v = json::serialize(obj, doc);
    return doc;
}

template <typename T>
rapidjson::Value serialize(const T& obj, rapidjson::Document& doc)
{
    return detail::serialize(obj, doc, priority_tag<2>{});
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
