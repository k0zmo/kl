#pragma once

#include "kl/type_traits.hpp"

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>

#include <typeinfo>

namespace kl {
namespace detail {

template <typename Class, typename MemberData>
struct make_const
{
    using type = MemberData;
};

template <typename Class, typename MemberData>
struct make_const<const Class, MemberData>
{
    using type = std::add_const_t<MemberData>;
};

template <typename Parent, typename MemberData>
class field_type_info
{
public:
    using original_type = MemberData;
    using class_type = Parent;
    // If Parent is const we make type also a const
    using type = typename make_const<Parent, MemberData>::type;

public:
    field_type_info(const char* name) : name_{name} {}
    const char* name() const { return name_; }

private:
    const char* name_;
};

template <typename Parent, typename MemberData>
class field_info : public field_type_info<Parent, MemberData>
{
    using super_type = field_type_info<Parent, MemberData>;

public:
    using original_type = typename super_type::original_type;
    using class_type = typename super_type::class_type;
    using type = typename super_type::type;

public:
    field_info(type& ref, const char* name) : super_type{name}, ref_{ref} {}

    type& get() { return ref_; }
    std::add_const_t<type&> get() const { return ref_; }

private:
    type& ref_;
};
} // namespace detail

template <typename... Ts>
struct type_pack : std::integral_constant<std::size_t, sizeof...(Ts)>
{
    template <std::size_t N>
    struct extract
    {
        using type = at_type_t<N, Ts...>;
    };
};

template <typename T>
struct type_info
{
    using this_type = T;
    using base_types = type_pack<>;

    static const char* name() { return typeid(T).name(); }
    static const char* full_name() { return typeid(T).name(); }
    static const bool is_reflectable = false;
    static const std::size_t num_fields = 0;

    template <typename Type, typename Visitor>
    static void reflect(Type&& obj, Visitor&& visitor)
    {
        static_assert(sizeof(Type) < 0, "Can't reflect this type");
    }

    template <typename Type, typename Visitor>
    static void reflect(Visitor&& visitor)
    {
        static_assert(sizeof(Type) < 0, "Can't reflect this type");
    }
};

template <typename T>
using is_reflectable =
    std::integral_constant<bool, type_info<std::decay_t<T>>::is_reflectable>;

namespace detail {

constexpr std::size_t base_num_fields(type_pack<>) { return 0; }

template <typename Head, typename... Tail>
constexpr std::size_t base_num_fields(type_pack<Head, Tail...>);
} // namespace detail

struct ctti
{
    template <typename Reflectable, typename Visitor>
    static void reflect(Reflectable&& r, Visitor&& visitor)
    {
        using non_ref = std::decay_t<Reflectable>;
        type_info<non_ref>::reflect(std::forward<Reflectable>(r),
                                    std::forward<Visitor>(visitor));
    }

    template <typename Reflectable, typename Visitor>
    static void reflect(Visitor&& visitor)
    {
        using non_ref = std::decay_t<Reflectable>;
        type_info<non_ref>::template reflect<Reflectable>(
            std::forward<Visitor>(visitor));
    }

    template <typename Reflectable>
    using base_types = typename type_info<Reflectable>::base_types;

    template <typename Reflectable>
    static const char* name()
    {
        return type_info<Reflectable>::name();
    }

    template <typename Reflectable>
    static const char* full_name()
    {
        return type_info<Reflectable>::full_name();
    }

    template <typename Reflectable>
    static constexpr bool is_reflectable()
    {
        return type_info<Reflectable>::is_reflectable;
    }

    template <typename Reflectable>
    static constexpr std::size_t num_fields()
    {
        return type_info<Reflectable>::num_fields;
    }

    template <typename Reflectable>
    static constexpr std::size_t total_num_fields()
    {
        return ctti::num_fields<Reflectable>() +
               detail::base_num_fields(ctti::base_types<Reflectable>());
    }
};

namespace detail {
template <typename Head, typename... Tail>
constexpr std::size_t base_num_fields(type_pack<Head, Tail...>)
{
    return ctti::total_num_fields<Head>() +
           base_num_fields(type_pack<Tail...>());
}
} // namespace detail
} // namespace kl

#if defined(_MSC_VER) && BOOST_VERSION == 105700
#undef BOOST_PP_EXPAND_I
#define BOOST_PP_EXPAND_I(...) __VA_ARGS__
#endif

#define KL_DEFINE_REFLECTABLE(...) KL_TYPE_INFO_IMPL(__VA_ARGS__)
#define KL_DEFINE_REFLECTABLE_DERIVED(...)                                     \
    KL_TYPE_INFO_DERIVED_IMPL(__VA_ARGS__)

#if defined(_MSC_VER) && !defined(__INTELLISENSE__)
#define KL_TYPE_INFO_IMPL(...)                                                 \
    BOOST_PP_CAT(                                                              \
        BOOST_PP_OVERLOAD(KL_TYPE_INFO_IMPL_, __VA_ARGS__)(__VA_ARGS__),       \
        BOOST_PP_EMPTY())
#define KL_TYPE_INFO_DERIVED_IMPL(...)                                         \
    BOOST_PP_CAT(BOOST_PP_OVERLOAD(KL_TYPE_INFO_DERIVED_IMPL_,                 \
                                   __VA_ARGS__)(__VA_ARGS__),                  \
                 BOOST_PP_EMPTY())
#else
#define KL_TYPE_INFO_IMPL(...)                                                 \
    BOOST_PP_OVERLOAD(KL_TYPE_INFO_IMPL_, __VA_ARGS__)(__VA_ARGS__)
#define KL_TYPE_INFO_DERIVED_IMPL(...)                                         \
    BOOST_PP_OVERLOAD(KL_TYPE_INFO_DERIVED_IMPL_, __VA_ARGS__)(__VA_ARGS__)
#endif

// for types defined in global namespace
#define KL_TYPE_INFO_IMPL_2(name_, values_)                                    \
    KL_TYPE_INFO_DEFINITION(_, name_, _, values_)

// for types defined in some namespace(s)
#define KL_TYPE_INFO_IMPL_3(ns_, name_, values_)                               \
    KL_TYPE_INFO_DEFINITION(KL_TYPE_INFO_ARG_TO_TUPLE(ns_), name_, _, values_)

// for types defined in global namespace, deriving from some base(s)
#define KL_TYPE_INFO_DERIVED_IMPL_3(name_, bases_, values_)                    \
    KL_TYPE_INFO_DEFINITION(_, name_, KL_TYPE_INFO_ARG_TO_TUPLE(bases_),       \
                            values_)

// for types defined in some namespace(s), deriving from some base(s)
#define KL_TYPE_INFO_DERIVED_IMPL_4(ns_, name_, bases_, values_)               \
    KL_TYPE_INFO_DEFINITION(KL_TYPE_INFO_ARG_TO_TUPLE(ns_), name_,             \
                            KL_TYPE_INFO_ARG_TO_TUPLE(bases_), values_)

#define KL_TYPE_INFO_DEFINITION(ns_, name_, bases_, values_)                   \
    KL_TYPE_INFO_DEFINITION_IMPL(KL_TYPE_INFO_FULL_NAME(ns_, name_),           \
                                 KL_TYPE_INFO_FULL_NAME_STRING(ns_, name_),    \
                                 name_, bases_, values_)

#define KL_TYPE_INFO_DEFINITION_IMPL(full_name_, full_name_string_, name_,     \
                                     bases_, values_)                          \
    namespace kl {                                                             \
    template <>                                                                \
    struct type_info<full_name_>                                               \
    {                                                                          \
        using this_type = full_name_;                                          \
        using base_types = type_pack<KL_TYPE_INFO_GET_BASE_TYPES(bases_)>;     \
                                                                               \
        static const char* name() { return BOOST_PP_STRINGIZE(name_); }        \
        static const char* full_name() { return full_name_string_; }           \
        static const bool is_reflectable = true;                               \
                                                                               \
        static const std::size_t num_fields =                                  \
            KL_TYPE_INFO_GET_FIELD_COUNT(values_);                             \
                                                                               \
        template <typename Type, typename Visitor>                             \
        static void reflect(Type&& obj, Visitor&& visitor)                     \
        {                                                                      \
            KL_TYPE_INFO_VISIT_WITH_FIELD_INFO_BASES(bases_)                   \
            KL_TYPE_INFO_VISIT_WITH_FIELD_INFO(values_)                        \
        }                                                                      \
                                                                               \
        template <typename Type, typename Visitor>                             \
        static void reflect(Visitor&& visitor)                                 \
        {                                                                      \
            KL_TYPE_INFO_VISIT_WITH_FIELD_TYPE_INFO_BASES(bases_)              \
            KL_TYPE_INFO_VISIT_WITH_FIELD_TYPE_INFO(values_)                   \
        }                                                                      \
    };                                                                         \
    }

#define KL_TYPE_INFO_GET_BASE_TYPES_COUNT(bases_)                              \
    BOOST_PP_IF(BOOST_PP_IS_BEGIN_PARENS(bases_),                              \
                BOOST_PP_TUPLE_SIZE(bases_),                                   \
                0)

#define KL_TYPE_INFO_GET_BASE_TYPES_IMPL(base_) base_

#define KL_TYPE_INFO_GET_BASE_TYPES(bases_)                                    \
    BOOST_PP_ENUM(KL_TYPE_INFO_GET_BASE_TYPES_COUNT(bases_),                   \
                  KL_TYPE_INFO_FOR_EACH_IN_TUPLE,                              \
                  (bases_, KL_TYPE_INFO_GET_BASE_TYPES_IMPL))

#define KL_TYPE_INFO_GET_FIELD_COUNT(values_)                                  \
    BOOST_PP_IF(BOOST_PP_IS_BEGIN_PARENS(values_),                             \
                BOOST_PP_TUPLE_SIZE(values_), 0)

#define KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(values_, macro_)                     \
    BOOST_PP_EXPR_IF(BOOST_PP_IS_BEGIN_PARENS(values_),                        \
                     BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(values_),             \
                                     KL_TYPE_INFO_FOR_EACH_IN_TUPLE,           \
                                     (values_, macro_)))

#define KL_TYPE_INFO_VISIT_WITH_FIELD_INFO_BASES_IMPL(base_)                   \
    type_info<base_>::reflect(std::forward<Type>(obj),                         \
                              std::forward<Visitor>(visitor));

#define KL_TYPE_INFO_VISIT_WITH_FIELD_INFO_BASES(bases_)                       \
    KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(                                         \
        bases_, KL_TYPE_INFO_VISIT_WITH_FIELD_INFO_BASES_IMPL)

#define KL_TYPE_INFO_VISIT_WITH_FIELD_TYPE_INFO_BASES_IMPL(base_)              \
    type_info<base_>::reflect<Type>(std::forward<Visitor>(visitor));

#define KL_TYPE_INFO_VISIT_WITH_FIELD_TYPE_INFO_BASES(bases_)                  \
    KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(                                         \
        bases_, KL_TYPE_INFO_VISIT_WITH_FIELD_TYPE_INFO_BASES_IMPL)

#define KL_TYPE_INFO_VISIT_WITH_FIELD_INFO_IMPL(name_)                         \
    std::forward<Visitor>(visitor)(                                            \
        detail::field_info<std::remove_reference_t<Type>,                      \
                           decltype(this_type::name_)>{obj.name_,              \
                                                  BOOST_PP_STRINGIZE(name_)});

#define KL_TYPE_INFO_VISIT_WITH_FIELD_INFO(values_)                            \
    KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(values_,                                 \
                                      KL_TYPE_INFO_VISIT_WITH_FIELD_INFO_IMPL)

#define KL_TYPE_INFO_VISIT_WITH_FIELD_KL_TYPE_INFO_IMPL(name_)                 \
    std::forward<Visitor>(visitor)(                                            \
        detail::field_type_info<std::remove_reference_t<Type>,                 \
                                decltype(this_type::name_)>{                   \
            BOOST_PP_STRINGIZE(name_)});

#define KL_TYPE_INFO_VISIT_WITH_FIELD_TYPE_INFO(values_)                       \
    KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(                                         \
        values_, KL_TYPE_INFO_VISIT_WITH_FIELD_KL_TYPE_INFO_IMPL)

// makes sure arg is a tuple (works for tuples and single arg)
#define KL_TYPE_INFO_ARG_TO_TUPLE(arg_) (BOOST_PP_REMOVE_PARENS(arg_))

// tuple_macro is ((tuple), macro)
#define KL_TYPE_INFO_FOR_EACH_IN_TUPLE(_, index_, tuple_macro_)                \
    BOOST_PP_TUPLE_ELEM(1, tuple_macro_)                                       \
    (BOOST_PP_TUPLE_ELEM(index_, BOOST_PP_TUPLE_ELEM(0, tuple_macro_)))

#define KL_TYPE_INFO_NAMESPACE_SCOPE(ns_) ns_::

// Construct type's full name (with namespace scope)
#define KL_TYPE_INFO_FULL_NAME(ns_, name_)                                     \
    KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(ns_, KL_TYPE_INFO_NAMESPACE_SCOPE)       \
    name_

#define KL_TYPE_INFO_NAMESPACE_SCOPE_STRING(ns_) BOOST_PP_STRINGIZE(ns_::)

// Constructs type's full name (with namespace scope) as a string
#define KL_TYPE_INFO_FULL_NAME_STRING(ns_, name_)                              \
    KL_TYPE_INFO_FOR_EACH_IN_TUPLE_IF(ns_,                                     \
                                      KL_TYPE_INFO_NAMESPACE_SCOPE_STRING)     \
    BOOST_PP_STRINGIZE(name_)
