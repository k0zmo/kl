#pragma once

#include "kl/type_class.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/index_sequence.hpp"
#include "kl/tuple.hpp"

#include <boost/optional.hpp>
#include <json11/json11.hpp>
#include <exception>

KL_DEFINE_ENUM_REFLECTOR(json11::Json::Type,
                         (NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT))

namespace kl {

struct json_deserialize_exception : std::exception
{
    explicit json_deserialize_exception(const char* message)
        : json_deserialize_exception(std::string(message))
    {
    }
    explicit json_deserialize_exception(std::string message)
        : messages_(std::move(message))
    {
    }

    char const* what() const noexcept override { return messages_.c_str(); }

    void add(const char* message)
    {
        messages_.insert(end(messages_), '\n');
        messages_.append(message);
    }

private:
    std::string messages_;
};

// Forward declarations for top-level functions: to- and from- json conversions
template <typename T>
json11::Json to_json(const T& t);
template <typename T>
T from_json(const json11::Json& json);

namespace detail {

using ::kl::type_class::is_optional;
using ::kl::type_class::is_vector;
using ::kl::type_class::is_tuple;

// Checks if we can construct a Json object with given T
template <typename T>
using is_json_constructible =
    std::integral_constant<bool,
                           // Sadly, Json is constructible from boost::optional
                           // but causes compilation error
                           std::is_constructible<json11::Json, T>::value &&
                               !is_optional<T>::value && !std::is_enum<T>::value
#if defined(_MSC_VER) && (_MSC_VER < 1900)
                               // MSVC2013 has some issues with deleted ctor
                               // and is_constructible<T> trait
                               && (!std::is_convertible<T, void*>::value ||
                                   std::is_same<T, std::nullptr_t>::value)
#endif
                           >;

template <typename T, typename = void_t<>>
struct is_string_associative : std::false_type {};
template <typename T>
struct is_string_associative<
    T, void_t<std::enable_if_t<
           type_class::equals<T, type_class::map>::value &&
           std::is_constructible<std::string, typename T::key_type>::value>>>
    : std::true_type {};

template <typename T>
using is_representable_as_double =
    std::integral_constant<bool,
                           std::is_integral<T>::value && (sizeof(T) <= 4)>;

struct json_factory
{
    // T is integral type with no more bytes than 4. Effectively, it's u32 as
    // all other cases are covered with is_json_constructible<T> 'true branch'
    template <typename T, enable_if<is_representable_as_double<T>> = 0>
    static json11::Json create(const T& t)
    {
        if (static_cast<int>(t) >= 0)
            return to_json(static_cast<int>(t));
        return to_json(static_cast<double>(t));
    }

    // T is an enum type (with enum_reflector defined)
    template <typename T, enable_if<is_enum_reflectable<T>> = 0>
    static json11::Json create(const T& t)
    {
        auto str = enum_reflector<T>::to_string(t);
        return str ? to_json(str) : to_json(nullptr);
    }

    // T is an enum type
    template <typename T, enable_if<is_enum_nonreflectable<T>> = 0>
    static json11::Json create(const T& t)
    {
        return to_json(static_cast<std::underlying_type_t<T>>(t));
    }

    // T is optional<U>
    template <typename T, enable_if<is_optional<T>> = 0>
    static json11::Json create(const T& t)
    {
        if (t)
            return to_json(t.get());
        return to_json(nullptr);
    }

    // T is a class type that is reflectable
    template <typename T, enable_if<is_reflectable<T>> = 0>
    static json11::Json create(const T& t)
    {
        json11::Json::object obj;
        ctti::reflect(t, json_obj_serializer{obj});
        return json11::Json{std::move(obj)};
    }

    // T is vector-like container of simple json types or types that are reflectable
    template <typename T, enable_if<is_vector<T>> = 0>
    static json11::Json create(const T& vec)
    {
        json11::Json::array array_;
        array_.reserve(vec.size());
        for (const auto& item : vec)
            array_.push_back(to_json(item));
        return json11::Json{std::move(array_)};
    }

    // T is map-like container of simple json types or types that are reflectable
    template <typename T, enable_if<is_string_associative<T>> = 0>
    static json11::Json create(const T& map)
    {
        json11::Json::object object_;
        for (const auto& kv : map)
            object_.emplace(kv.first, to_json(kv.second));
        return json11::Json{std::move(object_)};
    }

    // T is a tuple<Ts...>
    template <typename T, enable_if<is_tuple<T>> = 0>
    static json11::Json create(const T& t)
    {
        return create_from_tuple(t, make_tuple_indices<T>{});
    }

private:
    // Visitor for reflectable types
    struct json_obj_serializer
    {
        explicit json_obj_serializer(json11::Json::object& obj) : obj_{obj} {}

        template <typename FieldInfo>
        void operator()(FieldInfo f)
        {
#if !defined(KL_JSON_DONT_SKIP_NULL_VALUES)
            if (is_optional<
                    std::remove_const_t<typename FieldInfo::type>>::value)
            {
                auto j = to_json(f.get());
                if (!j.is_null())
                    obj_.emplace(f.name(), std::move(j));
            }
            else
#endif
            {
                obj_.emplace(f.name(), to_json(f.get()));
            }
        }

    private:
        json11::Json::object& obj_;
    };

private:
    template <typename T, std::size_t... Is>
    static json11::Json create_from_tuple(const T& tuple, index_sequence<Is...>)
    {
        // Builds array of Json's from the given tuple serializing each element
        return json11::Json{{to_json(std::get<Is>(tuple))...}};
    }
};

// Dispatch function when T can't be used directly for Json object construction
template <typename T>
json11::Json to_json_impl(const T& t, std::false_type /*is_json_constructible*/)
{
    return json_factory::create(t);
}

// Dispatch function when T can be used directly for Json object construction
template <typename T>
json11::Json to_json_impl(const T& t, std::true_type /*is_json_constructible*/)
{
    // easy case, T can be use to construct Json directly
    return json11::Json{t};
}

// from_json implementation

inline std::string json_type_name(const json11::Json& json)
{
    return {enum_reflector<json11::Json::Type>::to_string(json.type())};
}

template <typename T>
using is_json_simple = std::integral_constant<
    bool,
    std::is_integral<T>::value || std::is_floating_point<T>::value ||
        std::is_enum<T>::value ||
        (std::is_convertible<std::string, T>::value && !is_optional<T>::value)>;

template <typename T, enable_if<std::is_integral<T>> = 0>
T from_json_simple(const json11::Json& json)
{
    if (!json.is_number())
        throw json_deserialize_exception{"type must be an integral but is " +
                                         json_type_name(json)};
    // This check will be ellided by the compiler since it's a constant expr
    if (std::is_unsigned<T>::value && sizeof(T) == 4)
        return static_cast<T>(json.number_value());
    return static_cast<T>(json.int_value());
}

template <>
inline bool from_json_simple<bool>(const json11::Json& json)
{
    if (!json.is_bool())
        throw json_deserialize_exception{"type must be a bool but is " +
                                         json_type_name(json)};
    return json.bool_value();
}

template <typename T, enable_if<std::is_floating_point<T>> = 0>
T from_json_simple(const json11::Json& json)
{
    if (!json.is_number())
        throw json_deserialize_exception{"type must be a float but is " +
                                         json_type_name(json)};
    return static_cast<T>(json.number_value());
}

template <typename T, enable_if<is_enum_reflectable<T>> = 0>
T from_json_simple(const json11::Json& json)
{
    if (!json.is_string())
        throw json_deserialize_exception{"type must be a string-enum but is " +
                                         json_type_name(json)};
    if (auto enum_value = enum_reflector<T>::from_string(json.string_value()))
        return enum_value.get();
    throw json_deserialize_exception{"invalid enum value: " +
                                     json.string_value()};
}

template <typename T, enable_if<is_enum_nonreflectable<T>> = 0>
T from_json_simple(const json11::Json& json)
{
    if (!json.is_number())
        throw json_deserialize_exception{"type must be a number-enum but is " +
                                         json_type_name(json)};
    return static_cast<T>(json.int_value());
}

template <typename T, enable_if<std::is_convertible<std::string, T>> = 0>
T from_json_simple(const json11::Json& json)
{
    if (!json.is_string())
        throw json_deserialize_exception{"type must be a string but is " +
                                         json_type_name(json)};
    return json.string_value();
}

// Same as is_string_associative byt with arguments swapped in is_constructible<> type trait
template <typename T, typename = void_t<>>
struct is_associative_string : std::false_type {};
template <typename T>
struct is_associative_string<
    T, void_t<std::enable_if_t<
           type_class::equals<T, type_class::map>::value &&
           std::is_constructible<typename T::key_type, std::string>::value>>>
    : std::true_type {};

// Checks if this expression is valid for given T class-type:
// T::from_json(const json11::Json&) -> T
template <typename T>
using from_json_memfun_type =
    decltype(T::from_json(std::declval<const json11::Json&>()));
template <typename T, typename = void_t<>>
struct is_constructible_from_json : std::false_type {};
template <typename T>
struct is_constructible_from_json<
    T, void_t<from_json_memfun_type<T>,
                  enable_if<std::is_same<T, from_json_memfun_type<T>>>>>
    : std::true_type {};

struct value_factory
{
    // T is optional<U>
    template <typename T, enable_if<is_optional<T>> = 0>
    static T create(const json11::Json& json)
    {
        if (json.is_null())
            return T{};
        return from_json<typename T::value_type>(json);
    }

    // T is a class type that is reflectable and default constructible
    template <typename T, enable_if<is_reflectable<T>,
                                    std::is_default_constructible<T>> = 0>
    static T create(const json11::Json& json)
    {
        T obj{};

        if (json.is_object())
        {
            json_obj_deserializer visitor{json.object_items()};
            ctti::reflect(obj, visitor);
        }
        else if (json.is_array())
        {
            if (json.array_items().size() != ctti::total_num_fields<T>())
            {
                throw json_deserialize_exception{"array size is different than "
                                                 "declared struct's field "
                                                 "count"};
            }
            json_obj_array_deserializer visitor{json.array_items()};
            ctti::reflect(obj, visitor);
        }
        else
        {
            throw json_deserialize_exception{
                "type must be an array or object but is " +
                json_type_name(json)};
        }

        return obj;
    }

    // T is a class type with a static member function: T from_json(const json11::Json&)
    template <typename T, enable_if<is_constructible_from_json<T>> = 0>
    static T create(const json11::Json& json)
    {
        return T::from_json(json);
    }

    // T is vector-like container of simple json types or types that are reflectable
    template <typename T, enable_if<is_vector<T>> = 0>
    static T create(const json11::Json& json)
    {
        if (!json.is_array())
            throw json_deserialize_exception{"type must be an array but is " +
                                             json_type_name(json)};

        T ret{};
        ret.reserve(json.array_items().size());

        for (const auto& item : json.array_items())
        {
            try
            {
                ret.push_back(from_json<typename T::value_type>(item));
            }
            catch (json_deserialize_exception& ex)
            {
                std::string msg = "error when deserializing element " +
                                  std::to_string(ret.size());
                ex.add(msg.c_str());
                throw;
            }
        }

        return ret;
    }

    // T is map-like container of simple json types or types that are reflectable
    template <typename T, enable_if<is_associative_string<T>> = 0>
    static T create(const json11::Json& json)
    {
        if (!json.is_object())
            throw json_deserialize_exception{"type must be an object but is " +
                                             json_type_name(json)};

        T ret{};

        for (const auto& obj : json.object_items())
        {
            try
            {
                ret.emplace(obj.first,
                            from_json<typename T::mapped_type>(obj.second));
            }
            catch (json_deserialize_exception& ex)
            {
                std::string msg = "error when deserializing field " + obj.first;
                ex.add(msg.c_str());
                throw;
            }
        }

        return ret;
    }

    // T is a tuple<Ts...>
    template <typename T, enable_if<is_tuple<T>> = 0>
    static T create(const json11::Json& json)
    {
        if (!json.is_array())
            throw json_deserialize_exception{"type must be an array but is " +
                                             json_type_name(json)};
        if (json.array_items().size() != std::tuple_size<T>::value)
            throw json_deserialize_exception{
                "array size is different than tuple size"};

        return from_json_tuple<T>(json, make_tuple_indices<T>{});
    }

private:
    // Visitor for reflectable types
    struct json_obj_deserializer
    {
        explicit json_obj_deserializer(const json11::Json::object& obj)
            : obj_{obj}
        {
        }

        template <typename FieldInfo>
        void operator()(FieldInfo f)
        {
            const auto it = obj_.find(f.name());
            if (it != obj_.cend())
            {
                try
                {
                    f.get() = from_json<typename FieldInfo::type>(it->second);
                }
                catch (json_deserialize_exception& ex)
                {
                    std::string msg = "error when deserializing field " +
                                      std::string(f.name());
                    ex.add(msg.c_str());
                    throw;
                }
                return;
            }
            else if (is_optional<typename FieldInfo::type>::value)
            {
                // optional fields doesnt have to be present in json
                f.get() = typename FieldInfo::type{};
                return;
            }

            throw json_deserialize_exception{"missing field " +
                                             std::string(f.name())};
        }

    private:
        const json11::Json::object& obj_;
    };

    struct json_obj_array_deserializer
    {
        explicit json_obj_array_deserializer(const json11::Json::array& arr)
            : arr_{arr}, index_{0}
        {
        }

        template <typename FieldInfo>
        void operator()(FieldInfo f)
        {
            f.get() = from_json<typename FieldInfo::type>(arr_[index_++]);
        }

    private:
        const json11::Json::array& arr_;
        size_t index_;
    };

private:
    template <typename T, std::size_t... Is>
    static T from_json_tuple(const json11::Json& json, index_sequence<Is...>)
    {
        const auto& arr = json.array_items();
        return std::make_tuple(
            from_json<typename std::tuple_element<Is, T>::type>(arr[Is])...);
    }
};

// Dispatch function when T cannot be created from given Json
template <typename T>
T from_json_impl(const json11::Json& json, std::false_type /*is_json_simple*/)
{
    return value_factory::create<T>(json);
}

// Dispatch function when T can be "almost" directly created from given Json
template <typename T>
T from_json_impl(const json11::Json& json, std::true_type /*is_json_simple*/)
{
    return from_json_simple<T>(json);
}
} // namespace detail

// Top-level functions
template <typename T>
json11::Json to_json(const T& t)
{
    return detail::to_json_impl(t, detail::is_json_constructible<T>{});
}

template <typename T>
T from_json(const json11::Json& json)
{
    return detail::from_json_impl<T>(json, detail::is_json_simple<T>{});
}
} // namespace kl
