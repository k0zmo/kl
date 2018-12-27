#pragma once

#include "kl/type_traits.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_flags.hpp"
#include "kl/tuple.hpp"
#include "kl/utility.hpp"

#include <boost/optional.hpp>
#include <json11.hpp>

#include <exception>

KL_DEFINE_ENUM_REFLECTOR(json11, Json::Type,
                         (NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT))

namespace kl {
namespace json {

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

template <typename T>
struct serializer;

template <typename T>
json11::Json serialize(const T& obj);

template <typename T>
T deserialize(type_t<T>, const json11::Json& j);

// Shorter version of from which can't be overloaded. Only use to invoke
// the from() without providing a bit weird first parameter.
template <typename T>
T deserialize(const json11::Json& j)
{
    // TODO replace with variable template type<T>
    constexpr static auto type = type_t<T>{};
    return json::deserialize(type, j);
}

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

// Checks if we can construct a Json object with given T
template <typename T>
using is_json_constructible = std::integral_constant<
    bool, std::is_constructible<json11::Json, T>::value &&
              // We want reflectable unscoped enum to handle ourselves
              !std::is_enum<T>::value>;

KL_VALID_EXPR_HELPER(has_reserve, std::declval<T&>().reserve(0U))
KL_VALID_EXPR_HELPER(has_from_json,
                     T::from_json(std::declval<const json11::Json&>()))

// For all T's that we can directly create json11::Json value from
template <typename JsonConstructible,
          enable_if<is_json_constructible<JsonConstructible>> = true>
json11::Json to_json(const JsonConstructible& value)
{
    return {value};
}

// For all T's that quacks like a std::map
template <typename Map, enable_if<negation<is_json_constructible<Map>>,
                                  is_map_alike<Map>> = true>
json11::Json to_json(const Map& map)
{
    static_assert(
        std::is_constructible<std::string, typename Map::key_type>::value,
        "std::string must be constructible from the Map's key type");

    json11::Json::object obj;
    for (const auto& kv : map)
        obj.emplace(kv.first, json::serialize(kv.second));
    return {std::move(obj)};
}

// For all T's that quacks like a std::vector
template <typename Vector, enable_if<negation<is_json_constructible<Vector>>,
                                     negation<is_map_alike<Vector>>,
                                     is_vector_alike<Vector>> = true>
json11::Json to_json(const Vector& vec)
{
    json11::Json::array array;
    array.reserve(vec.size());
    for (const auto& item : vec)
        array.push_back(json::serialize(item));
    return {std::move(array)};
}

// For all T's for which there's a type_info defined
template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
json11::Json to_json(const Reflectable& refl)
{
    json11::Json::object obj;
    ctti::reflect(refl, [&obj](auto fi) {
        auto json = json::serialize(fi.get());
        // We don't include fields with null as value in returned JSON object
        if (!json.is_null())
            obj.emplace(fi.name(), std::move(json));
    });
    return {std::move(obj)};
}

template <typename Enum>
json11::Json enum_to_json(Enum e, std::true_type /*is_enum_reflectable*/)
{
    return json::serialize(enum_reflector<Enum>::to_string(e));
}

template <typename Enum>
json11::Json enum_to_json(Enum e, std::false_type /*is_enum_reflectable*/)
{
    return json::serialize(underlying_cast(e));
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
json11::Json to_json(Enum e)
{
    return enum_to_json(e, is_enum_reflectable<Enum>{});
}

template <typename Enum>
json11::Json to_json(const enum_flags<Enum>& flags)
{
    static_assert(is_enum_reflectable<Enum>::value,
                  "Only flags of reflectable enums are supported");
    json11::Json::array arr;

    for (const auto possible_values : enum_reflector<Enum>::values())
    {
        if (flags.test(possible_values))
            arr.push_back(enum_reflector<Enum>::to_string(possible_values));
    }

    return {arr};
}

template <typename Tuple, std::size_t... Is>
json11::Json tuple_to_json(const Tuple& tuple, index_sequence<Is...>)
{
    return {{json::serialize(std::get<Is>(tuple))...}};
}

template <typename... Ts>
json11::Json to_json(const std::tuple<Ts...>& tuple)
{
    return tuple_to_json(tuple, make_index_sequence<sizeof...(Ts)>{});
}

inline json11::Json to_json(std::uint32_t value)
{
    if (static_cast<int>(value) >= 0)
        return json::serialize(static_cast<int>(value));
    return json::serialize(static_cast<double>(value));
}

template <typename T>
json11::Json to_json(const boost::optional<T>& opt)
{
    if (opt)
        return json::serialize(*opt);
    return json::serialize(nullptr);
}

// from_json implementation

inline std::string json_type_name(const json11::Json& json)
{
    return {enum_reflector<json11::Json::Type>::to_string(json.type())};
}

template <typename Integral, enable_if<std::is_integral<Integral>> = true>
Integral from_json(type_t<Integral>, const json11::Json& json)
{
    if (!json.is_number())
        throw deserialize_error{"type must be an integral but is " +
                                json_type_name(json)};
    // This check will be ellided by the compiler since it's a constant expr
    if (std::is_unsigned<Integral>::value && sizeof(Integral) == 4)
        return static_cast<Integral>(json.number_value());
    return static_cast<Integral>(json.int_value());
}

template <typename Floating, enable_if<std::is_floating_point<Floating>> = true>
Floating from_json(type_t<Floating>, const json11::Json& json)
{
    if (!json.is_number())
        throw deserialize_error{"type must be a float but is " +
                                json_type_name(json)};
    return static_cast<Floating>(json.number_value());
}

inline std::string from_json(type_t<std::string>, const json11::Json& json)
{
    if (!json.is_string())
        throw deserialize_error{"type must be a string but is " +
                                json_type_name(json)};
    return json.string_value();
}

inline bool from_json(type_t<bool>, const json11::Json& json)
{
    if (!json.is_bool())
        throw deserialize_error{"type must be a bool but is " +
                                json_type_name(json)};
    return json.bool_value();
}

template <typename HasFromJson, enable_if<has_from_json<HasFromJson>> = true>
HasFromJson from_json(type_t<HasFromJson>, const json11::Json& json)
{
    return HasFromJson::from_json(json);
}

template <typename Map, enable_if<is_map_alike<Map>> = true>
Map from_json(type_t<Map>, const json11::Json& json)
{
    static_assert(
        std::is_constructible<std::string, typename Map::key_type>::value,
        "Map's key type must be constructible from the std::string");

    if (!json.is_object())
        throw deserialize_error{"type must be an object but is " +
                                json_type_name(json)};

    Map ret{};

    for (const auto& obj : json.object_items())
    {
        try
        {
            ret.emplace(obj.first, json::deserialize<typename Map::mapped_type>(
                                       obj.second));
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing field " + obj.first;
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
Vector from_json(type_t<Vector>, const json11::Json& json)
{
    if (!json.is_array())
        throw deserialize_error{"type must be an array but is " +
                                json_type_name(json)};

    Vector ret{};
    vector_reserve(ret, json.array_items().size(), has_reserve<Vector>{});

    for (const auto& item : json.array_items())
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
inline const json11::Json safe_get_json(const json11::Json::array& json_array,
                                        std::size_t idx)
{
    static const auto null_value = json11::Json{};
    return idx >= json_array.size() ? null_value : json_array[idx];
}

template <typename Reflectable>
Reflectable reflectable_from_json(const json11::Json& json)
{
    Reflectable refl{};

    if (json.is_object())
    {
        const auto& obj = json.object_items();
        ctti::reflect(refl, [&obj](auto fi) {
            using field_type = typename decltype(fi)::type;

            try
            {
                const auto it = obj.find(fi.name());
                if (it != obj.cend())
                    fi.get() = json::deserialize<field_type>(it->second);
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
    else if (json.is_array())
    {
        if (json.array_items().size() > ctti::total_num_fields<Reflectable>())
        {
            throw deserialize_error{"array size is greater than "
                                    "declared struct's field "
                                    "count"};
        }
        const auto& arr = json.array_items();
        ctti::reflect(refl, [&arr, index = 0U](auto fi) mutable {
            using field_type = typename decltype(fi)::type;

            try
            {
                const auto& j = safe_get_json(arr, index);
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
                                json_type_name(json)};
    }

    return refl;
}

template <typename Reflectable, enable_if<is_reflectable<Reflectable>> = true>
Reflectable from_json(type_t<Reflectable>, const json11::Json& json)
{
    static_assert(std::is_default_constructible<Reflectable>::value,
                  "Reflectable must be default constructible");

    try
    {
        return reflectable_from_json<Reflectable>(json);
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
static Tuple tuple_from_json(const json11::Json& json, index_sequence<Is...>)
{
    const auto& arr = json.array_items();
    return std::make_tuple(json::deserialize<std::tuple_element_t<Is, Tuple>>(
        safe_get_json(arr, Is))...);
}

template <typename... Ts>
std::tuple<Ts...> from_json(type_t<std::tuple<Ts...>>, const json11::Json& json)
{
    if (!json.is_array())
        throw deserialize_error{"type must be an array but is " +
                                json_type_name(json)};

    return tuple_from_json<std::tuple<Ts...>>(
        json, make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
boost::optional<T> from_json(type_t<boost::optional<T>>,
                             const json11::Json& json)
{
    if (json.is_null())
        return {};
    return json::deserialize<T>(json);
}

template <typename Enum>
Enum enum_from_json(const json11::Json& json,
                    std::false_type /*is_enum_reflectable*/)
{
    if (!json.is_number())
        throw deserialize_error{"type must be a number-enum but is " +
                                json_type_name(json)};
    return static_cast<Enum>(json.int_value());
}

template <typename Enum>
Enum enum_from_json(const json11::Json& json,
                    std::true_type /*is_enum_reflectable*/)
{
    if (!json.is_string())
        throw deserialize_error{"type must be a string-enum but is " +
                                json_type_name(json)};
    if (auto enum_value =
            enum_reflector<Enum>::from_string(json.string_value()))
    {
        return enum_value.get();
    }
    throw deserialize_error{"invalid enum value: " + json.string_value()};
}

template <typename Enum, enable_if<std::is_enum<Enum>> = true>
Enum from_json(type_t<Enum>, const json11::Json& json)
{
    return enum_from_json<Enum>(json, is_enum_reflectable<Enum>{});
}

template <typename Enum>
enum_flags<Enum> from_json(type_t<enum_flags<Enum>>, const json11::Json& json)
{
    if (!json.is_array())
        throw deserialize_error{"type must be an array but is " +
                                json_type_name(json)};

    enum_flags<Enum> ret{};

    for (const auto& value : json.array_items())
    {
        const auto e = json::deserialize<Enum>(value);
        ret |= e;
    }

    return ret;
}

template <typename T>
json11::Json serialize(const T& obj, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot serialize an instance of type T - no viable "
                  "definition of to_json provided");
    return {}; // Keeps compiler happy
}

template <typename T>
auto serialize(const T& obj, priority_tag<1>) -> decltype(to_json(obj))
{
    return to_json(obj);
}

template <typename T>
auto serialize(const T& obj, priority_tag<2>)
    -> decltype(json::serializer<T>::to_json(obj))
{
    return json::serializer<T>::to_json(obj);
}

template <typename T>
T deserialize(const json11::Json& json, priority_tag<0>)
{
    static_assert(always_false<T>::value,
                  "Cannot deserialize an instance of type T - no viable "
                  "definition of from_json provided");
    return T{}; // Keeps compiler happy
}

template <typename T>
auto deserialize(const json11::Json& json, priority_tag<1>)
    -> decltype(from_json(std::declval<type_t<T>>(), json))
{
    // TODO replace with variable template type<T>
    constexpr static auto type = type_t<T>{};
    return from_json(type, json);
}

template <typename T>
auto deserialize(const json11::Json& json, priority_tag<2>)
    -> decltype(json::serializer<T>::from_json(json))
{
    return json::serializer<T>::from_json(json);
}
} // namespace detail

// Top-level functions
template <typename T>
json11::Json serialize(const T& obj)
{
    // Dispatch method based on:
    // https://quuxplusone.github.io/blog/2018/03/19/customization-points-for-functions/
    return detail::serialize(obj, priority_tag<2>{});
}

template <typename T>
T deserialize(type_t<T>, const json11::Json& json)
{
    return detail::deserialize<T>(json, priority_tag<2>{});
}
} // namespace json
} // namespace kl

inline json11::Json operator ""_json(const char* s, std::size_t)
{
    std::string err;
    auto j = json11::Json::parse(s, err);
    if (!err.empty())
        throw kl::json::parse_error{std::move(err)};
    return j;
}
