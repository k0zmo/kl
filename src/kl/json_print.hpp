#pragma once

#include "kl/type_traits.hpp"
#include "kl/enum_traits.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/ctti.hpp"

#include <string>
#include <vector>
#include <boost/optional.hpp>

namespace kl {

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& t);

namespace detail {
namespace type_class {

struct boolean {};
struct integral {};
struct floating {};
struct enumeration {};
struct string {};
struct reflectable {};
template <typename Value>
struct optional { using value = Value;  };
template <typename Value>
struct list { using value = Value;  };
template <typename Key, typename Value>
struct map { using key = Key; using value = Value; };

template <typename T>
struct is_bool : std::false_type {};
template <>
struct is_bool<bool> : std::true_type {};

template <typename T>
struct is_string : std::false_type {};
template <typename Char, typename CharTraits, typename Allocator>
struct is_string<std::basic_string<Char, CharTraits, Allocator>>
    : std::true_type
{
};
template <std::size_t N>
struct is_string<char[N]> : std::true_type {};

template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<boost::optional<T>> : std::true_type {};
template <typename T>
struct is_optional<T*> : std::true_type {};

template <typename T>
struct is_list : std::false_type {};
template <typename T, typename Allocator>
struct is_list<std::vector<T, Allocator>> : std::true_type {};

template <typename T>
struct get_impl
{
    using type = std::conditional_t<
        is_bool<T>::value, boolean,
        std::conditional_t<
            std::is_integral<T>::value, integral,
            std::conditional_t<
                std::is_floating_point<T>::value, floating,
                std::conditional_t<
                    std::is_enum<T>::value, enumeration,
                    std::conditional_t<
                        is_string<T>::value, string,
                        std::conditional_t<
                            is_list<T>::value, list<T>,
                            std::conditional_t<
                                is_reflectable<T>::value, reflectable,
                                std::conditional_t<is_optional<T>::value,
                                                   optional<T>, T>>>>>>>>;
};

template <typename T>
using get = typename get_impl<T>::type;
} // namespace type_class

// Default implementation (integral, floating)
template <typename /*Class*/>
struct json_printer
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << value;
        return os;
    }
};

template <>
struct json_printer<type_class::boolean>
{
    template <typename OutStream>
    static OutStream& print(OutStream& os, const bool value)
    {
        os << (value ? "true" : "false");
        return os;
    }
};

template <>
struct json_printer<type_class::enumeration>
{
    template <typename OutStream, typename T,
              enable_if<negation<is_enum_reflectable<T>>> = 0>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << underlying_cast(value);
        return os;
    }

    template <typename OutStream, typename T,
              enable_if<is_enum_reflectable<T>> = 0>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << '"' << enum_reflector<T>::to_string(value) << '"';
        return os;
    }
};

template <>
struct json_printer<type_class::string>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << '"' << value << '"';
        return os;
    }
};

template <>
struct json_printer<type_class::reflectable>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << '{';
        ctti::reflect(value, object_printer<OutStream>{os});
        os << '}';
        return os;
    }

private:
    template <typename OutStream>
    struct object_printer
    {
        explicit object_printer(OutStream& os) : os_{os} {}

        template <typename FieldInfo>
        void operator()(FieldInfo f)
        {
#if !defined(KL_JSON_DONT_SKIP_NULL_VALUES)
            call_op(std::move(f),
                    type_class::is_optional<
                        std::remove_const_t<typename FieldInfo::type>>{});
#else

            call_op(std::move(f), std::false_type{});
#endif
        }

    private:
        template <typename FieldInfo>
        void call_op(FieldInfo f, std::false_type /*is_optional*/)
        {
            call_op(std::move(f));
        }

        template <typename FieldInfo>
        void call_op(FieldInfo f, std::true_type /*is_optional*/)
        {
            if (!!f.get())
                call_op(std::move(f));
        }

        template <typename FieldInfo>
        void call_op(FieldInfo f)
        {
            if (!first_)
                os_ << ',';
            else
                first_ = false;

            os_ << '"' << f.name() << "\":";
            json_print(os_, f.get());
        }

    private:
        OutStream& os_;
        bool first_{true};
    };
};

template <typename Value>
struct json_printer<type_class::optional<Value>>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        if (!!value)
            json_print(os, *value);
        else
            os << "null";
        return os;
    }
};

template <typename ElemValue>
struct json_printer<type_class::list<ElemValue>>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& list)
    {
        os << '[';
        bool first = true;
        for (const auto& value : list)
        {
            if (!first)
                os << ',';
            else
                first = false;
            json_print(os, value);
        }
        os << ']';
        return os;
    }
};
} // namespace detail

// ### TODO: json_pretty_print

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value)
{
    using type_class = detail::type_class::get<T>;
    return detail::json_printer<type_class>::print(os, value);
}
} // namespace kl
