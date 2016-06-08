#pragma once

#include <boost/optional.hpp>
#include <json11/json11.hpp>

#include "kl/enum_reflector.hpp"
#include "kl/ctti.hpp"
#include "kl/index_sequence.hpp"
#include "kl/tuple.hpp"

namespace kl {

// Forward declarations for top-level functions: to- and from- json conversions
template <typename T>
json11::Json to_json(const T& t);
template <typename T>
boost::optional<T> from_json(const json11::Json& json);

namespace detail {

template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<boost::optional<T>> : std::true_type {};

// Checks if we can construct a Json object with given T
template <typename T>
using is_json_constructible =
    std::integral_constant<bool,
                           // Sadly, Json is constructible from boost::optional
                           // but causes compilation error
                           std::is_constructible<json11::Json, T>::value &&
                               !is_optional<T>::value && !std::is_enum<T>::value
#if _MSC_VER < 1900
                               // MSVC2013 has some issues with deleted ctor
                               // and is_constructible<T> trait
                               && (!std::is_convertible<T, void*>::value ||
                                   std::is_same<T, std::nullptr_t>::value)
#endif
                           >;

// To serialize T we just need to be sure T models Container concept
// We can create as many specialization as we want (it's never enough),
// work out some template voodoo/black magic or
// let's just assume vector is the choice in 98,3% of cases
template <typename T>
struct is_vector : std::false_type {};
template <typename T>
struct is_vector<std::vector<T>> : std::true_type {};
// To deserialize we must constaint T a bit more: push_back() + reverse() optionally
// So let's just stick to std::vector.

template <typename T, typename = void_t<>>
struct is_string_associative : std::false_type {};
template <typename T>
struct is_string_associative<
    T, void_t<typename T::mapped_type,
              std::enable_if_t<std::is_constructible<
                  std::string, typename T::key_type>::value>>> : std::true_type
{};

template <typename T>
struct is_tuple : std::false_type {};
template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};

struct json_factory
{
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
#if !defined(KL_JSON_CONVERT_DONT_SKIP_NULL_VALUES)
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

template <typename T>
using is_json_simple = std::integral_constant<
    bool, std::is_integral<T>::value || std::is_floating_point<T>::value ||
              std::is_enum<T>::value ||
              std::is_convertible<std::string, T>::value>;

template <typename T, enable_if<std::is_integral<T>> = 0>
boost::optional<T> from_json_simple(const json11::Json& json)
{
    return json.is_number()
               ? boost::optional<T>{static_cast<T>(json.int_value())}
               : boost::none;
}

template <>
inline boost::optional<bool> from_json_simple<bool>(const json11::Json& json)
{
    return json.is_bool() ? boost::optional<bool>{json.bool_value()}
                          : boost::none;
}

template <typename T, enable_if<std::is_floating_point<T>> = 0>
boost::optional<T> from_json_simple(const json11::Json& json)
{
    return json.is_number()
               ? boost::optional<T>{static_cast<T>(json.number_value())}
               : boost::none;
}

template <typename T, enable_if<is_enum_reflectable<T>> = 0>
boost::optional<T> from_json_simple(const json11::Json& json)
{
    return json.is_string() ? boost::optional<T>{enum_reflector<T>::from_string(
                                  json.string_value())}
                            : boost::none;
}

template <typename T, enable_if<is_enum_nonreflectable<T>> = 0>
boost::optional<T> from_json_simple(const json11::Json& json)
{
    return json.is_number()
               ? boost::optional<T>{static_cast<T>(json.int_value())}
               : boost::none;
}

template <typename T, enable_if<std::is_convertible<std::string, T>> = 0>
boost::optional<T> from_json_simple(const json11::Json& json)
{
    return json.is_string() ? boost::optional<T>{json.string_value()}
                            : boost::none;
}

// Same as is_string_associative byt with arguments swapped in is_constructible<> type trait
template <typename T, typename = void_t<>>
struct is_associative_string : std::false_type {};
template <typename T>
struct is_associative_string<
    T, void_t<typename T::mapped_type,
              std::enable_if_t<std::is_constructible<typename T::key_type,
                                                     std::string>::value>>>
    : std::true_type {};

struct value_factory
{
    // T is optional<U>
    template <typename T, enable_if<is_optional<T>> = 0>
    static boost::optional<T> create(const json11::Json& json)
    {
        return from_json<typename T::value_type>(json);
    }

    // T is a class type that is reflectable
    template <typename T, enable_if<is_reflectable<T>> = 0>
    static boost::optional<T> create(const json11::Json& json)
    {
        boost::optional<T> obj = T{};
        json_obj_deserializer visitor{json.object_items()};
        ctti::reflect(obj.get(), visitor);
        if (!visitor.all_fields())
            obj = boost::none;
        return obj;
    }

    // T is vector-like container of simple json types or types that are reflectable
    template <typename T, enable_if<is_vector<T>> = 0>
    static boost::optional<T> create(const json11::Json& json)
    {
        boost::optional<T> ret = T{};
        ret->reserve(json.array_items().size());

        for (const auto& item : json.array_items())
        {
            if (auto value = from_json<typename T::value_type>(item))
            {
                ret->push_back(std::move(value).get());
            }
            else
            {
                ret = boost::none;
                return ret;
            }
        }

        return ret;
    }

    // T is map-like container of simple json types or types that are reflectable
    template <typename T, enable_if<is_associative_string<T>> = 0>
    static boost::optional<T> create(const json11::Json& json)
    {
        boost::optional<T> ret = T{};

        for (const auto& obj : json.object_items())
        {
            if (auto value = from_json<typename T::mapped_type>(obj.second))
            {
                ret->emplace(obj.first, std::move(value).get());
            }
            else
            {
                ret = boost::none;
                return ret;
            }
        }

        return ret;
    }

    // T is a tuple<Ts...>
    template <typename T, enable_if<is_tuple<T>> = 0>
    static boost::optional<T> create(const json11::Json& json)
    {
        return json.is_array()
                   ? json.array_items().size() == std::tuple_size<T>::value
                         ? from_json_tuple<T>(json, make_tuple_indices<T>{})
                         : boost::none
                   : boost::none;
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
            if (!all_fields_)
                return;

            const auto it = obj_.find(f.name());
            if (it != obj_.cend())
            {
                if (auto value =
                        from_json<typename FieldInfo::type>(it->second))
                {
                    f.get() = std::move(value).get();
                    return;
                }
            }
            else if (is_optional<typename FieldInfo::type>::value)
            {
                f.get() = typename FieldInfo::type{};
                return;
            }

            all_fields_ = false;
        }

        bool all_fields() const { return all_fields_; }

    private:
        const json11::Json::object& obj_;
        bool all_fields_{true};
    };

private:
    template <typename T, std::size_t... Is>
    static boost::optional<T> from_json_tuple(const json11::Json& json,
                                              index_sequence<Is...>)
    {
        const auto& arr = json.array_items();
        // deserialize all elements from array into tuple of optionals
        auto tuple_opt = std::make_tuple(
            from_json<typename std::tuple_element<Is, T>::type>(arr[Is])...);
        // check if all of optionals are initialized
        if (tuple::all_true_fn::call(tuple_opt))
            return tuple::transform_deref_fn::call(tuple_opt);
        return boost::none;
    }
};

// Dispatch function when T cannot be created from given Json
template <typename T>
boost::optional<T> from_json_impl(const json11::Json& json,
                                  std::false_type /*is_json_simple*/)
{
    return value_factory::create<T>(json);
}

// Dispatch function when T can be "almost" directly created from given Json
template <typename T>
boost::optional<T> from_json_impl(const json11::Json& json,
                                  std::true_type /*is_json_simple*/)
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
boost::optional<T> from_json(const json11::Json& json)
{
    return detail::from_json_impl<T>(json, detail::is_json_simple<T>{});
}
} // namespace kl
