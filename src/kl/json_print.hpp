#pragma once

#include "kl/type_class.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/tuple.hpp"

namespace kl {

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value);

namespace detail {

// Default implementation (integral, floating)
template <typename TypeClass>
struct json_printer
{
    static_assert(!std::is_same<TypeClass, type_class::unknown>::value, "!!!");

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

template <>
struct json_printer<type_class::tuple>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << '[';
        print_tuple(os, value, make_tuple_indices<T>{});
        os << ']';
        return os;
    }

private:
    template <typename OutStream, typename T, std::size_t... Is>
    static void print_tuple(OutStream& os, const T& value,
                            index_sequence<Is...>)
    {
        using swallow = std::initializer_list<int>;
        (void)swallow{
            (print_comma<Is>(os), json_print(os, std::get<Is>(value)), 0)...};
    }

    template <std::size_t Is, typename OutStream>
    static int print_comma(OutStream& os)
    {
        if (Is)
            os << ',';
        return 0;
    }
};

template <>
struct json_printer<type_class::optional>
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

template <>
struct json_printer<type_class::vector>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        os << '[';
        bool first = true;
        for (const auto& elem : value)
        {
            if (!first)
                os << ',';
            else
                first = false;
            json_print(os, elem);
        }
        os << ']';
        return os;
    }
};

template <>
struct json_printer<type_class::map>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value)
    {
        using key_type = typename T::key_type;
        static_assert(std::is_same<kl::type_class::string,
                                   kl::type_class::get<key_type>>::value,
                      "Key type must be a String");

        os << '{';
        bool first = true;
        for (const auto& kv : value)
        {
            if (!first)
                os << ',';
            else
                first = false;

            os << '"' << kv.first << "\":";
            json_print(os, kv.second);
        }
        os << '}';
        return os;
    }
};
} // namespace detail

// ### TODO: json_pretty_print

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value)
{
    return detail::json_printer<type_class::get<T>>::print(os, value);
}
} // namespace kl
