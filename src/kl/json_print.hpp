#pragma once

#include "kl/type_class.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/tuple.hpp"

namespace kl {

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value, int indent = 0);

namespace detail {

struct pretty_state
{
    int indent;
    int current_indent;

    template <typename OutStream>
    void mark_begin(OutStream& os, char begin_char)
    {
        if (indent <= 0)
        {
            os << begin_char;
        }
        else
        {
            current_indent += indent;
            os << begin_char << '\n';
        }
    }

    template <typename OutStream>
    void mark_end(OutStream& os, char end_char)
    {
        if (indent <= 0)
        {
            os << end_char;
        }
        else
        {
            os << '\n';
            current_indent -= indent;
            for (int i = 0; i < current_indent; ++i)
                os << ' ';
            os << end_char;
        }
    }

    template <typename OutStream>
    void mark_end_element(OutStream& os)
    {
        if (indent <= 0)
        {
            os << ',';
        }
        else
        {
            os << ",\n";
        }
    }

    template <typename OutStream>
    void mark_indent(OutStream& os)
    {
        for (int i = 0; i < current_indent; ++i)
            os << ' ';
    }
};

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value, pretty_state& state);

// Default implementation (integral, floating)
template <typename TypeClass>
struct json_printer
{
    static_assert(!std::is_same<TypeClass, type_class::unknown>::value,
                  "Can't print unknown type class");

    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state&)
    {
        os << value;
        return os;
    }
};

template <>
struct json_printer<type_class::boolean>
{
    template <typename OutStream>
    static OutStream& print(OutStream& os, const bool value, pretty_state&)
    {
        os << (value ? "true" : "false");
        return os;
    }
};

template <>
struct json_printer<type_class::enumeration>
{
    template <typename OutStream, typename T,
              enable_if<negation<is_enum_reflectable<T>>> = true>
    static OutStream& print(OutStream& os, const T& value, pretty_state&)
    {
        os << underlying_cast(value);
        return os;
    }

    template <typename OutStream, typename T,
              enable_if<is_enum_reflectable<T>> = true>
    static OutStream& print(OutStream& os, const T& value, pretty_state&)
    {
        os << '"' << enum_reflector<T>::to_string(value) << '"';
        return os;
    }
};

template <>
struct json_printer<type_class::string>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state&)
    {
        os << '"' << value << '"';
        return os;
    }
};

template <>
struct json_printer<type_class::reflectable>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state& state)
    {
        state.mark_begin(os, '{');
        ctti::reflect(value, object_printer<OutStream>{os, state});
        state.mark_end(os, '}');
        return os;
    }

private:
    template <typename OutStream>
    struct object_printer
    {
        explicit object_printer(OutStream& os, pretty_state& state)
            : os_{os}, state_{state}
        {
        }

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
                state_.mark_end_element(os_);
            else
                first_ = false;

            state_.mark_indent(os_);
            os_ << '"' << f.name() << (state_.indent > 0 ? "\": " : "\":");
            json_print(os_, f.get(), state_);
        }

    private:
        OutStream& os_;
        pretty_state& state_;
        bool first_{true};
    };
};

template <>
struct json_printer<type_class::tuple>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state& state)
    {
        state.mark_begin(os, '[');
        print_tuple(os, value, state, make_tuple_indices<T>{});
        state.mark_end(os, ']');
        return os;
    }

private:
    template <typename OutStream, typename T, std::size_t... Is>
    static void print_tuple(OutStream& os, const T& value, pretty_state& state,
                            index_sequence<Is...>)
    {
        using swallow = std::initializer_list<int>;
        (void)swallow{(mark_end_element<Is>(os, state),
                       json_print(os, std::get<Is>(value), state), 0)...};
    }

    template <std::size_t Is, typename OutStream>
    static int mark_end_element(OutStream& os, pretty_state& state)
    {
        if (Is)
            state.mark_end_element(os);
        state.mark_indent(os);
        return 0;
    }
};

template <>
struct json_printer<type_class::optional>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state& state)
    {
        if (!!value)
            json_print(os, *value, state);
        else
            os << "null";
        return os;
    }
};

template <>
struct json_printer<type_class::vector>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state& state)
    {
        state.mark_begin(os, '[');
        bool first = true;
        for (const auto& elem : value)
        {
            if (!first)
                state.mark_end_element(os);
            else
                first = false;

            state.mark_indent(os);
            json_print(os, elem, state);
        }
        state.mark_end(os, ']');
        return os;
    }
};

template <>
struct json_printer<type_class::map>
{
    template <typename OutStream, typename T>
    static OutStream& print(OutStream& os, const T& value, pretty_state& state)
    {
        using key_type = typename T::key_type;
        static_assert(std::is_same<kl::type_class::string,
                                   kl::type_class::get<key_type>>::value,
                      "Key type must be a String");

        state.mark_begin(os, '{');
        bool first = true;
        for (const auto& kv : value)
        {
            if (!first)
                state.mark_end_element(os);
            else
                first = false;

            state.mark_indent(os);
            os << '"' << kv.first << (state.indent > 0 ? "\": " : "\":");
            json_print(os, kv.second, state);
        }
        state.mark_end(os, '}');
        return os;
    }
};

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value, pretty_state& state)
{
    return detail::json_printer<type_class::get<T>>::print(os, value, state);
}
} // namespace detail

template <typename T, typename OutStream>
OutStream& json_print(OutStream& os, const T& value, int indent)
{
    detail::pretty_state state{indent, 0};
    return detail::json_print(os, value, state);
}
} // namespace kl
