#pragma once

#include "kl/type_traits.hpp"
#include "kl/reflect_struct.hpp"

#include <boost/core/demangle.hpp>

#include <cstddef>
#include <string>
#include <tuple>
#include <typeinfo>
#include <utility>

namespace kl::ctti {

// Evaluates whether two attribute matches
template <typename T, typename U>
inline constexpr bool attribute_matches_v =
    std::is_same_v<T, U> || std::is_convertible_v<const U*, const T*>;

namespace detail {

// Shared storage and attribute support for reflected runtime fields and
// object-free field descriptors.
template <typename... Attributes>
class field_data
{
public:
    constexpr field_data(const char* name, Attributes... attributes)
        : name_{name}, attributes_{std::move(attributes)...}
    {
    }

    constexpr const char* name() const noexcept
    {
        return name_;
    }

    template <typename Attribute>
    static constexpr bool has()
    {
        return (attribute_matches_v<Attribute, Attributes> || ...);
    }

    template <typename Attribute>
    constexpr const Attribute* get() const
    {
        return get_impl<Attribute, 0>();
    }

    template <typename Visitor>
    constexpr void visit_attributes(Visitor&& vis) const
    {
        std::apply([&](const auto&... attr) { (vis(attr), ...); }, attributes_);
    }

private:
    template <typename Attribute, std::size_t I>
    constexpr const Attribute* get_impl() const
    {
        if constexpr (I == sizeof...(Attributes))
        {
            return nullptr;
        }
        else
        {
            using current = std::tuple_element_t<I, std::tuple<Attributes...>>;

            if constexpr (attribute_matches_v<Attribute, current>)
            {
                return &std::get<I>(attributes_);
            }
            else
            {
                return get_impl<Attribute, I + 1>();
            }
        }
    }

private:
    const char* name_;
    std::tuple<Attributes...> attributes_;
};

template <typename Object, typename... Attributes>
class field_base : public field_data<Attributes...>
{
public:
    constexpr field_base(Object& object, const char* name, Attributes... attributes)
        : field_data<Attributes...>{name, std::move(attributes)...}, object_{&object}
    {
    }

protected:
    constexpr Object& object() const noexcept
    {
        return *object_;
    }

private:
    Object* object_;
};

template <typename... Attributes>
class field_base<void, Attributes...> : public field_data<Attributes...>
{
public:
    using field_data<Attributes...>::field_data;
};

// Reflected direct data member.
template <auto Ptr, typename Object, typename... Attributes>
class field : public detail::field_base<Object, Attributes...>
{
public:
    using value_type = remove_cvref_t<typename member_pointer_traits<Ptr>::value_type>;

    using detail::field_base<Object, Attributes...>::field_base;

    constexpr decltype(auto) value() const { return ((this->object()).*Ptr); }
};

// We can't use CTAD deduction guides for class template with non-deduced
// template parameter (Ptr).
template <auto Ptr, typename Object, typename... Attributes>
constexpr auto make_field(Object& object, const char* name, Attributes... attributes)
{
    return field<Ptr, Object, Attributes...>{object, name, std::move(attributes)...};
}

// Reflected field backed by a callable accessor. Use this for cases that cannot
// be represented by a pointer-to-member, such as reference data members,
// flattened nested members, or computed/custom views.
template <typename Object, typename Accessor, typename... Attributes>
class accessor_field : public detail::field_base<Object, Attributes...>
{
public:
    using value_type = remove_cvref_t<decltype(
        std::declval<Accessor>()(std::declval<Object&>()))>;

    constexpr accessor_field(Object& object, const char* name, Accessor accessor,
                             Attributes... attributes)
        : detail::field_base<Object, Attributes...>{object, name, std::move(attributes)...},
          accessor_{std::move(accessor)}
    {
    }

    constexpr decltype(auto) value() const { return accessor_(this->object()); }

private:
    Accessor accessor_;
};

template <typename Object, typename Accessor, typename... Attributes>
accessor_field(Object&, const char*, Accessor, Attributes...)
    -> accessor_field<Object, Accessor, Attributes...>;

// Same as `field` but without a bound object reference.
template <auto Ptr, typename... Attributes>
class field_descriptor : public detail::field_base<void, Attributes...>
{
public:
    using value_type = remove_cvref_t<typename member_pointer_traits<Ptr>::value_type>;

    using detail::field_base<void, Attributes...>::field_base;
};

template <auto Ptr, typename... Attributes>
constexpr auto make_field_descriptor(const char* name, Attributes... attributes)
{
    return field_descriptor<Ptr, Attributes...>{name, std::move(attributes)...};
}

template <typename ValueType, typename... Attributes>
class accessor_field_descriptor : public detail::field_base<void, Attributes...>
{
public:
    using value_type = ValueType;

    constexpr accessor_field_descriptor(const char* name, Attributes... attributes)
        : detail::field_base<void, Attributes...>{name, std::move(attributes)...}
    {
    }
};

template <typename ValueType, typename... Attributes>
constexpr auto make_accessor_field_descriptor(const char* name, Attributes... attributes)
{
    return accessor_field_descriptor<ValueType, Attributes...>{name, std::move(attributes)...};
}

// Field factory that is used by ctti::reflect_type function.
template <typename Filter, typename Object>
class type_field_factory
{
public:
    // AttributeFactories are simple lambdas returning an single instance of attribute.
    // This is done in such way so the attributes are created/evaluated lazily and can be
    // filtered out if needed when calling reflect_type. For example, one of attribute can't be
    // evaluated in constexpr context so we can skip it and still do some compile-time calculation
    // based on the rest of attributes
    template <auto Ptr, typename... AttributeFactories>
    constexpr auto field(const char* name, AttributeFactories... factories) const
    {
        return std::apply(
            [name](auto&&... attrs) {
                return make_field_descriptor<Ptr>(name, std::forward<decltype(attrs)>(attrs)...);
            },
            filter_attributes(std::move(factories)...));
    }

    template <typename Accessor, typename... AttributeFactories>
    constexpr auto accessor(const char* name, Accessor accessor,
                            AttributeFactories... factories) const
    {
        // For type-only reflection, we don't need an instance of accessor, just a type it returns
        (void)accessor;
        using value_type =
            remove_cvref_t<decltype(std::declval<Accessor>()(std::declval<Object&>()))>;

        return std::apply(
            [name](auto&&... attrs) {
                return make_accessor_field_descriptor<value_type>(
                    name, std::forward<decltype(attrs)>(attrs)...);
            },
            filter_attributes(std::move(factories)...));
    }

private:
    // Returns a single element tuple if filter passed the attribute or empty one otherwise
    template <typename AttributeFactory>
    static constexpr auto filter_one_attribute(AttributeFactory&& factory)
    {
        using attribute_type = decltype(std::declval<AttributeFactory&>()());
        if constexpr (Filter::template keep<attribute_type>())
        {
            return std::tuple<attribute_type>{std::forward<AttributeFactory>(factory)()};
        }
        else
        {
            (void)factory;
            return std::tuple<>{};
        }
    }

    template <typename... AttributeFactories>
    static constexpr auto filter_attributes(AttributeFactories&&... factories)
    {
        // Returns a tuple of all attributes that were kept after applying a filter
        return std::tuple_cat(filter_one_attribute(std::move(factories))...);
    }
};

// Field factory that is used by ctti::reflect_object function. Instantiates field's (or
// accessor_field's) without doing any attribute filtering (there's no need).
template <typename Object>
class runtime_field_factory
{
public:
    constexpr explicit runtime_field_factory(Object& object) noexcept
        : object_{object}
    {
    }

    template <auto Ptr, typename... AttributeFactories>
    constexpr auto field(const char* name, AttributeFactories... factories) const
    {
        return make_field<Ptr>(object_, name, factories()...);
    }

    template <typename Accessor, typename... AttributeFactories>
    constexpr auto accessor(const char* name, Accessor accessor,
                            AttributeFactories... factories) const
    {
        return accessor_field{object_, name, std::move(accessor), factories()...};
    }

private:
    Object& object_;
};

KL_VALID_EXPR_HELPER(has_reflect_struct_fields,
                     reflect_struct_fields(0 /*factory*/, 0 /*visitor*/, ::kl::ctti::record<T>))

} // namespace detail

template <typename T>
using is_reflectable = detail::has_reflect_struct_fields<T>;

template <typename T>
inline constexpr bool is_reflectable_v = is_reflectable<T>::value;

template <typename Reflected>
std::string name()
{
    using drop_cv_ref = remove_cvref_t<Reflected>;
    const std::type_info& ti = typeid(drop_cv_ref);
    boost::core::scoped_demangled_name demangled{ti.name()};
    return std::string{demangled.get()};
}

template <typename Reflected, typename Visitor>
constexpr void reflect_object(Reflected&& r, Visitor&& v)
{
    using R = remove_cvref_t<Reflected>;

    static_assert(detail::has_reflect_struct_fields_v<R>,
                  "Can't reflect object of this type. Use KL_REFLECT_STRUCT or provide "
                  "reflect_struct_fields manually.");
    reflect_struct_fields(detail::runtime_field_factory{r}, std::forward<Visitor>(v), record<R>);
}

// Default attribute filter that retains every attribute
struct keep_all_attributes
{
    template <typename>
    static constexpr bool keep() noexcept
    {
        return true;
    }
};

template <typename Reflected, typename Filter = keep_all_attributes,
          typename Visitor>
constexpr void reflect_type(Visitor&& v)
{
    using R = remove_cvref_t<Reflected>;

    static_assert(
        is_reflectable_v<R>,
        "Can't reflect type. Use KL_REFLECT_STRUCT or provide reflect_struct_fields manually.");
    reflect_struct_fields(detail::type_field_factory<Filter, R>{}, std::forward<Visitor>(v),
                          record<R>);
}

template <typename Reflected>
constexpr std::size_t num_fields() noexcept
{
    using R = remove_cvref_t<Reflected>;

    static_assert(detail::has_reflect_struct_fields_v<R>,
                  "Can't reflect this type. Use KL_REFLECT_STRUCT or provide reflect_struct_fields "
                  "manually.");
    return reflect_num_fields(record<R>);
}

template <typename Reflectable, typename... Attributes>
constexpr bool has_any_attribute()
{
    bool result = false;
    ctti::reflect_type<Reflectable>([&result](auto field) {
        using field_type = decltype(field);
        if constexpr((field_type::template has<Attributes>() || ...))
        {
            result = true;
        }
    });
    return result;
}

} // namespace kl::ctti
