#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/serialization_error.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_set.hpp"
#include "kl/serialization_attributes.hpp"
#include "kl/serialization_fwd.hpp"
#include "kl/type_traits.hpp"
#include "kl/utility.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kl::serialization::detail {

template <typename Attribute, typename Field>
constexpr bool has_attribute(const Field&)
{
    return Field::template has<Attribute>();
}

template <typename Field>
constexpr const char* serialized_name(const Field& field)
{
    if constexpr (Field::template has<attributes::rename_t>())
        return field.template get<attributes::rename_t>()->name;
    else
        return field.name();
}

template <typename Field, typename Value, typename Context>
bool skip_null_field(const Value& value, Context& ctx)
{
    if constexpr (Field::template has<attributes::emit_null_t>())
        return false;
    else if constexpr (Field::template has<attributes::skip_if_null_t>())
        return is_null_value(value);
    else
        return ctx.skip_null_value(value);
}

template <typename Field, typename Value>
bool skip_empty_field(const Value& value)
{
    if constexpr (Field::template has<attributes::skip_if_empty_t>())
        return is_empty_value(value);
    else
        return false;
}

template <typename Field, typename Value, typename Context>
bool skip_serialized_field(const Value& value, Context& ctx)
{
    return skip_null_field<Field>(value, ctx) || skip_empty_field<Field>(value);
}

template <typename Field>
constexpr void check_flatten_field()
{
    using value_type = typename Field::value_type;

    if constexpr (Field::template has<attributes::flatten_t>())
    {
        static_assert(ctti::is_reflectable_v<value_type>,
                      "serialization flatten field must be reflectable");
    }
}

template <typename Field>
constexpr void check_extra_fields_field()
{
    using value_type = typename Field::value_type;

    if constexpr (Field::template has<attributes::extra_fields_t>())
    {
        static_assert(::kl::detail::is_map_alike<value_type>::value,
                      "serialization extra_fields field must be map-like");

        if constexpr (::kl::detail::is_map_alike<value_type>::value)
        {
            static_assert(
                std::is_constructible_v<std::string, typename value_type::key_type>,
                "serialization extra_fields key type must be constructible as std::string");
        }
    }
}

template <typename Field>
constexpr void check_field_attributes()
{
    check_flatten_field<Field>();
    check_extra_fields_field<Field>();
}

template <typename Field>
bool apply_default_value(const Field& field)
{
    bool applied = false;

    field.visit_attributes([&](const auto& attr) {
        using attr_type = std::decay_t<decltype(attr)>;

        if constexpr (attributes::detail::is_default_value_v<attr_type>)
        {
            using default_type =
                typename attributes::detail::is_default_value<attr_type>::value_type;

            static_assert(std::is_assignable_v<decltype(field.value()), default_type>,
                          "serialization default_value must be assignable to the field type");
            if (!applied)
            {
                field.value() = attr.value;
                applied = true;
            }
        }
    });

    return applied;
}

template <typename Field>
bool apply_missing_field_policy(const Field& field)
{
    bool applied = apply_default_value(field);
    if (!applied && field.template has<attributes::allow_missing_t>())
    {
        applied = true;
    }
    return applied;
}

template <typename Field>
bool apply_null_field_policy(const Field& field)
{
    return apply_default_value(field);
}

template <typename Backend, typename Field>
const char* resolve_field_name(const typename Backend::value_type& value, const Field& field)
{
    const char* canonical = serialized_name(field);

    if (Backend::has_field(value, canonical))
        return canonical;

    if constexpr (Field::template has<attributes::aliases_t>())
    {
        const auto* aliases = field.template get<attributes::aliases_t>();
        for (const char* alias : *aliases)
        {
            if (Backend::has_field(value, alias))
                return alias;
        }
    }

    return nullptr;
}

using validator_attribute_filter =
    ctti::any_attribute_filter<attributes::rename_t,
                               attributes::aliases_t,
                               attributes::flatten_t,
                               attributes::extra_fields_t,
                               attributes::skip_serialization_t,
                               attributes::skip_deserialization_t>;

// Small fixed-capacity bag of string literals with constexpr uniqueness checks.
template <std::size_t Capacity>
struct cstr_name_set
{
    const char* data[Capacity > 0 ? Capacity : 1]{};
    std::size_t size{0};

    constexpr bool push_unique(const char* name)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            if (cstring_equal(data[i], name))
                return false;
        }
        if (size >= Capacity)
            throw "kl::serialization: reflected field name buffer overflow";
        data[size++] = name;
        return true;
    }

private:
    constexpr bool cstring_equal(const char* a, const char* b) noexcept
    {
        while (*a && *a == *b)
        {
            ++a;
            ++b;
        }
        return *a == *b;
    }
};

template <typename Reflectable>
struct serialize_field_names_validator
{
    static_assert(ctti::is_reflectable_v<Reflectable>);

    static constexpr bool validate()
    {
        cstr_name_set<field_count<Reflectable>()> buf;
        return collect_field_names<Reflectable>(buf);
    }

private:
    template <typename T>
    static constexpr std::size_t field_count()
    {
        std::size_t n = 0;
        ctti::reflect_type<T, validator_attribute_filter>(
            [&n](auto field) { n += count_field_contribution<decltype(field)>(); });
        return n;
    }

    template <typename F>
    static constexpr std::size_t count_field_contribution()
    {
        if constexpr (F::template has<attributes::skip_serialization_t>() ||
                      F::template has<attributes::extra_fields_t>())
        {
            return 0;
        }
        else if constexpr (F::template has<attributes::flatten_t>())
        {
            static_assert(ctti::is_reflectable_v<typename F::value_type>,
                          "serialization flatten field must be reflectable");
            return field_count<typename F::value_type>();
        }
        else
        {
            return 1;
        }
    }

    template <typename T, typename Buf>
    static constexpr bool collect_field_names(Buf& buf)
    {
        bool result = true;
        ctti::reflect_type<T, validator_attribute_filter>([&buf, &result](auto field) {
            using F = decltype(field);
            if constexpr (F::template has<attributes::skip_serialization_t>() ||
                          F::template has<attributes::extra_fields_t>())
            {
            }
            else if constexpr (F::template has<attributes::flatten_t>())
            {
                if (!collect_field_names<typename F::value_type>(buf))
                    result = false;
            }
            else
            {
                if (!buf.push_unique(serialized_name(field)))
                    result = false;
            }
        });
        return result;
    }
};

template <typename Reflectable>
struct deserialize_field_names_validator
{
    static_assert(ctti::is_reflectable_v<Reflectable>);

    static constexpr bool validate()
    {
        cstr_name_set<field_count<Reflectable>()> buf;
        return collect_field_names<Reflectable>(buf);
    }

private:
    template <typename T>
    static constexpr std::size_t field_count()
    {
        std::size_t n = 0;
        ctti::reflect_type<T, validator_attribute_filter>(
            [&n](auto field) {
                n += count_field_contribution<decltype(field)>();
            });
        return n;
    }

    template <typename F>
    static constexpr std::size_t count_field_contribution()
    {
        if constexpr (F::template has<attributes::skip_deserialization_t>() ||
                      F::template has<attributes::extra_fields_t>())
        {
            return 0;
        }
        else if constexpr (F::template has<attributes::flatten_t>())
        {
            static_assert(ctti::is_reflectable_v<typename F::value_type>,
                          "serialization flatten field must be reflectable");
            return field_count<typename F::value_type>();
        }
        else if constexpr (F::template has<attributes::aliases_t>())
        {
            return 1 + attributes::aliases_t::max_aliases;
        }
        else
        {
            return 1;
        }
    }

    template <typename T, typename Buf>
    static constexpr bool collect_field_names(Buf& buf)
    {
        bool result = true;
        ctti::reflect_type<T, validator_attribute_filter>([&buf, &result](auto field) {
            using F = decltype(field);
            if constexpr (F::template has<attributes::skip_deserialization_t>() ||
                          F::template has<attributes::extra_fields_t>())
            {
            }
            else if constexpr (F::template has<attributes::flatten_t>())
            {
                if (!collect_field_names<typename F::value_type>(buf))
                    result = false;
            }
            else
            {
                if (!buf.push_unique(serialized_name(field)))
                    result = false;

                if constexpr (F::template has<attributes::aliases_t>())
                {
                    const auto* aliases = field.template get<attributes::aliases_t>();
                    for (const char* alias : *aliases)
                    {
                        if (!buf.push_unique(alias))
                            result = false;
                    }
                }
            }
        });
        return result;
    }
};

using string_set = std::set<std::string, std::less<>>;

template <typename Field>
void collect_field_reserved_names(string_set& names, std::size_t& extra_fields_count,
                                  const Field& field)
{
    if constexpr (Field::template has<attributes::extra_fields_t>())
    {
        ++extra_fields_count;
        if (extra_fields_count > 1)
        {
            throw std::logic_error{
                "serialization only one extra_fields field is supported"};
        }
        return;
    }
    else if constexpr (Field::template has<attributes::flatten_t>())
    {
        ctti::reflect_type<typename Field::value_type, validator_attribute_filter>(
            [&names, &extra_fields_count](auto nested_field) {
                collect_field_reserved_names(names, extra_fields_count, nested_field);
            });
    }
    else
    {
        // We want to detect duplicate reflected names/aliases and throw an error if needed
        auto add_name = [&names](const char* name) {
            if (!names.insert(name).second)
            {
                throw std::logic_error{"serialization reflected field name collision: " +
                                       std::string{name}};
            }
        };

        add_name(serialized_name(field));

        if constexpr (Field::template has<attributes::aliases_t>())
        {
            const auto* aliases = field.template get<attributes::aliases_t>();
            for (const char* alias : *aliases)
                add_name(alias);
        }
    }
}

// Build a list of field names but only if any of the fields has
// extra_field or flatten attribute attached to it
template <typename Reflectable>
const string_set* reserved_field_names()
{
    if constexpr (ctti::has_any_attribute<Reflectable, attributes::extra_fields_t,
                                          attributes::flatten_t>())
    {
        // Create (once per Reflectable type) a set<string>, consisting of 'reserved' field names
        // that attributes extra_fields or flatten can't collide with
        static const string_set names = [] {
            string_set result;
            std::size_t extra_fields_count = 0;
            ctti::reflect_type<Reflectable, validator_attribute_filter>(
                [&result, &extra_fields_count](auto field) {
                    check_field_attributes<decltype(field)>();
                    collect_field_reserved_names(result, extra_fields_count, field);
                });
            return result;
        }();
        return &names;
    }
    else
    {
        // if we dont have any extra_fields or flatten attributes - let's not pay the price for
        // building a set of 'reserved' field names
        return nullptr;
    }
}

template <typename Reflectable>
constexpr std::size_t sequence_deserializable_field_count()
{
    std::size_t n = 0;
    ctti::reflect_type<Reflectable, validator_attribute_filter>([&n](auto field) {
        using Field = decltype(field);
        if constexpr (!Field::template has<attributes::skip_deserialization_t>())
            ++n;
    });
    return n;
}

template <typename Key>
void check_extra_field_key(const string_set& reserved_names, const Key& key)
{
    static constexpr bool can_use_transparent_comparison =
        std::is_invocable_r_v<bool, std::less<>, const std::string&, const Key&> &&
        std::is_invocable_r_v<bool, std::less<>, const Key&, const std::string&>;

    if constexpr (can_use_transparent_comparison)
    {
        if (reserved_names.find(key) != reserved_names.end())
        {
            throw std::logic_error{
                "serialization extra_fields key collides with reflected field: " +
                std::string{key}};
        }
    }
    else
    {
        std::string key_name{key};
        if (reserved_names.find(key_name) != reserved_names.end())
        {
            throw std::logic_error{
                "serialization extra_fields key collides with reflected field: " + key_name};
        }
    }
}

template <typename Backend, typename Reflectable, typename Context>
void dump_reflected_fields(const Reflectable& refl,
                           const string_set* reserved_names,
                           Context& ctx)
{
    ctti::reflect_object(refl, [&reserved_names, &ctx](auto field) {
        check_field_attributes<decltype(field)>();

        if constexpr (!has_attribute<attributes::skip_serialization_t>(field))
        {
            auto&& value = field.value();
            if (!skip_serialized_field<decltype(field)>(value, ctx))
            {
                if constexpr (decltype(field)::template has<attributes::extra_fields_t>())
                {
                    for (const auto& [key, extra_value] : value)
                    {
                        check_extra_field_key(*reserved_names, key);

                        if (!ctx.skip_null_value(extra_value))
                        {
                            Backend::write_key(key, ctx);
                            Backend::dump(extra_value, ctx);
                        }
                    }
                }
                else if constexpr (decltype(field)::template has<attributes::flatten_t>())
                {
                    dump_reflected_fields<Backend>(value, reserved_names, ctx);
                }
                else
                {
                    Backend::write_key(serialized_name(field), ctx);
                    Backend::dump(value, ctx);
                }
            }
        }
    });
}

template <typename Backend, typename Reflectable, typename Context>
void add_reflected_fields(typename Backend::value_type& out,
                          const Reflectable& refl,
                          const string_set* reserved_names,
                          Context& ctx)
{
    ctti::reflect_object(refl, [&out, &reserved_names, &ctx](auto field) {
        check_field_attributes<decltype(field)>();

        if constexpr (!has_attribute<attributes::skip_serialization_t>(field))
        {
            auto&& value = field.value();
            if (!skip_serialized_field<decltype(field)>(value, ctx))
            {
                if constexpr (decltype(field)::template has<attributes::extra_fields_t>())
                {
                    for (const auto& [key, extra_value] : value)
                    {
                        check_extra_field_key(*reserved_names, key);

                        if (!ctx.skip_null_value(extra_value))
                        {
                            Backend::add_field(out,
                                               key,
                                               Backend::serialize(extra_value, ctx),
                                               ctx);
                        }
                    }
                }
                else if constexpr (decltype(field)::template has<attributes::flatten_t>())
                {
                    add_reflected_fields<Backend>(out, value, reserved_names, ctx);
                }
                else
                {
                    Backend::add_field(out,
                                       serialized_name(field),
                                       Backend::serialize(value, ctx),
                                       ctx);
                }
            }
        }
    });
}

// dump_adl implementation

template <typename Backend, typename Map, typename Context,
          enable_if<::kl::detail::is_map_alike<Map>> = true>
void dump_adl(const Map& map, Context& ctx)
{
    static_assert(std::is_constructible_v<std::string, typename Map::key_type>,
                  "std::string must be constructible from the Map's key type");

    Backend::begin_map(ctx);
    for (const auto& [key, value] : map)
    {
        if (!ctx.skip_null_value(value))
        {
            Backend::write_key(key, ctx);
            Backend::dump(value, ctx);
        }
    }
    Backend::end_map(ctx);
}

template <typename Backend, typename Range, typename Context,
          enable_if<std::negation<::kl::detail::is_map_alike<Range>>,
                    ::kl::detail::is_range<Range>> = true>
void dump_adl(const Range& rng, Context& ctx)
{
    Backend::begin_sequence(ctx);
    for (const auto& value : rng)
        Backend::dump(value, ctx);
    Backend::end_sequence(ctx);
}

template <typename Backend, typename Reflectable, typename Context,
          enable_if<ctti::is_reflectable<Reflectable>> = true>
void dump_adl(const Reflectable& refl, Context& ctx)
{
    static_assert(serialize_field_names_validator<Reflectable>::validate(),
                  "reflected serialization field name collision (duplicate canonical name)");
    
    Backend::begin_map(ctx);
    dump_reflected_fields<Backend>(refl, reserved_field_names<Reflectable>(), ctx);
    Backend::end_map(ctx);
}

template <typename Backend, typename Enum, typename Context,
          enable_if<std::is_enum<Enum>> = true>
void dump_adl(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        Backend::dump(kl::to_string(e), ctx);
    else
        Backend::dump(underlying_cast(e), ctx);
}

template <typename Backend, typename Enum, typename Context>
void dump_adl(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");

    Backend::begin_sequence(ctx);
    for (const auto possible_value : reflect_enum<Enum>().values())
    {
        if (set.test(possible_value))
            Backend::dump(kl::to_string(possible_value), ctx);
    }
    Backend::end_sequence(ctx);
}

namespace impl {

template <typename Backend, typename Tuple, typename Context, std::size_t... Is>
void dump_tuple(const Tuple& tuple, Context& ctx, std::index_sequence<Is...>)
{
    Backend::begin_sequence(ctx);
    (Backend::dump(std::get<Is>(tuple), ctx), ...);
    Backend::end_sequence(ctx);
}

} // namespace impl

template <typename Backend, typename... Ts, typename Context>
void dump_adl(const std::tuple<Ts...>& tuple, Context& ctx)
{
    impl::dump_tuple<Backend>(tuple, ctx,
                              std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Backend, typename T, typename Context>
void dump_adl(const std::optional<T>& opt, Context& ctx)
{
    if (!opt)
        Backend::dump(nullptr, ctx);
    else
        Backend::dump(*opt, ctx);
}

// serialize_adl implementation

template <typename Backend, typename Map, typename Context,
          enable_if<::kl::detail::is_map_alike<Map>> = true>
typename Backend::value_type serialize_adl(const Map& map, Context& ctx)
{
    static_assert(std::is_constructible_v<std::string, typename Map::key_type>,
                  "std::string must be constructible from the Map's key type");

    auto out = Backend::make_map();
    for (const auto& [key, value] : map)
    {
        if (!ctx.skip_null_value(value))
            Backend::add_field(out, key, Backend::serialize(value, ctx), ctx);
    }
    return out;
}

template <typename Backend, typename Range, typename Context,
          enable_if<std::negation<::kl::detail::is_map_alike<Range>>,
                    ::kl::detail::is_range<Range>> = true>
typename Backend::value_type serialize_adl(const Range& rng, Context& ctx)
{
    auto out = Backend::make_sequence();
    for (const auto& value : rng)
        Backend::add_element(out, Backend::serialize(value, ctx), ctx);
    return out;
}

template <typename Backend, typename Reflectable, typename Context,
          enable_if<ctti::is_reflectable<Reflectable>> = true>
typename Backend::value_type serialize_adl(const Reflectable& refl, Context& ctx)
{
    static_assert(serialize_field_names_validator<Reflectable>::validate(),
                  "reflected serialization field name collision (duplicate canonical name)");
    auto out = Backend::make_map();
    add_reflected_fields<Backend>(out, refl, reserved_field_names<Reflectable>(), ctx);
    return out;
}

template <typename Backend, typename Enum, typename Context, enable_if<std::is_enum<Enum>> = true>
typename Backend::value_type serialize_adl(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        return Backend::serialize(kl::to_string(e), ctx);
    else
        return Backend::serialize(underlying_cast(e), ctx);
}

template <typename Backend, typename Enum, typename Context>
typename Backend::value_type serialize_adl(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>, "Only sets of reflectable enums are supported");

    auto out = Backend::make_sequence();
    for (const auto possible_value : reflect_enum<Enum>().values())
    {
        if (set.test(possible_value))
        {
            Backend::add_element(out, Backend::serialize(possible_value, ctx), ctx);
        }
    }
    return out;
}

namespace impl {

template <typename Backend, typename Tuple, typename Context, std::size_t... Is>
typename Backend::value_type serialize_tuple(const Tuple& tuple, Context& ctx,
                                             std::index_sequence<Is...>)
{
    auto out = Backend::make_sequence();
    (Backend::add_element(out, Backend::serialize(std::get<Is>(tuple), ctx), ctx), ...);
    return out;
}

} // namespace impl

template <typename Backend, typename... Ts, typename Context>
typename Backend::value_type serialize_adl(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return impl::serialize_tuple<Backend>(tuple, ctx, std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Backend, typename T, typename Context>
typename Backend::value_type serialize_adl(const std::optional<T>& opt, Context& ctx)
{
    if (opt)
        return Backend::serialize(*opt, ctx);
    return typename Backend::value_type{};
}

// deserialize_adl implementation

template <typename Backend, typename Map, typename Context,
          enable_if<::kl::detail::is_map_alike<Map>> = true>
void deserialize_adl(Map& out, const typename Backend::value_type& value, Context& ctx)
{
    Backend::expect_map(value);

    out.clear();

    Backend::for_each_field(value, [&out, &ctx](const auto& key, const auto& field) {
        try
        {
            typename Map::key_type key_value{};
            Backend::deserialize(key_value, key, ctx);
            typename Map::mapped_type mapped_value{};
            Backend::deserialize(mapped_value, field, ctx);
            out.emplace(std::move(key_value), std::move(mapped_value));
        }
        catch (deserialize_error& ex)
        {
            std::string field_name;
            Backend::deserialize(field_name, key, ctx);
            std::string msg = "error when deserializing field " + field_name;
            ex.add(msg.c_str());
            throw;
        }
    });
}

template <typename Backend, typename GrowableRange, typename Context,
          enable_if<std::negation<::kl::detail::is_map_alike<GrowableRange>>,
                    ::kl::detail::is_growable_range<GrowableRange>> = true>
void deserialize_adl(GrowableRange& out, const typename Backend::value_type& value,
                     Context& ctx)
{
    Backend::expect_sequence(value);

    out.clear();
    if constexpr (::kl::detail::has_reserve_v<GrowableRange>)
        out.reserve(Backend::size(value));

    Backend::for_each_element(value, [&out, &ctx](const auto& item) {
        try
        {
            typename GrowableRange::value_type element{};
            Backend::deserialize(element, item, ctx);
            out.push_back(std::move(element));
        }
        catch (deserialize_error& ex)
        {
            std::string msg = "error when deserializing element " + std::to_string(out.size());
            ex.add(msg.c_str());
            throw;
        }
    });
}

template <typename Backend, typename Reflectable, typename Context,
          enable_if<ctti::is_reflectable<Reflectable>> = true>
void deserialize_adl(Reflectable& out, const typename Backend::value_type& value,
                     Context& ctx)
try
{
    static_assert(deserialize_field_names_validator<Reflectable>::validate(),
                  "reflected deserialization field name collision (duplicate canonical name)");

    if (Backend::is_map(value))
    {
        const auto* reserved_names = reserved_field_names<Reflectable>();

        ctti::reflect_object(out, [&value, &reserved_names, &ctx](auto field) {
            check_field_attributes<decltype(field)>();

            if constexpr (!has_attribute<attributes::skip_deserialization_t>(field))
            {
                using Field = decltype(field);
                if constexpr (Field::template has<attributes::extra_fields_t>())
                {
                    assert(reserved_names);
                    auto& extras = field.value();
                    extras.clear();

                    Backend::for_each_field(value, [&](const auto& key, const auto& node) {
                        std::string key_value;
                        bool key_deserialized = false;

                        try
                        {
                            Backend::deserialize(key_value, key, ctx);
                            key_deserialized = true;
                            if (reserved_names->find(key_value) != reserved_names->end())
                                return;

                            typename remove_cvref_t<decltype(extras)>::mapped_type mapped_value{};
                            Backend::deserialize(mapped_value, node, ctx);
                            extras.emplace(std::move(key_value), std::move(mapped_value));
                        }
                        catch (deserialize_error& ex)
                        {
                            std::string msg = "error when deserializing extra field ";
                            msg += key_deserialized ? key_value : "key";
                            ex.add(msg.c_str());
                            throw;
                        }
                    });
                    return;
                }

                try
                {
                    if constexpr (Field::template has<attributes::flatten_t>())
                    {
                        Backend::deserialize(field.value(), value, ctx);
                        return;
                    }

                    const char* name = resolve_field_name<Backend>(value, field);
                    if (!name)
                    {
                        if (apply_missing_field_policy(field))
                            return;

                        Backend::deserialize(field.value(),
                                             Backend::at_field(value, serialized_name(field)),
                                             ctx);
                        return;
                    }

                    const auto& node = Backend::at_field(value, name);
                    // If the node exists but is null and we also need to apply a default value.
                    if (Backend::is_null(node) && apply_null_field_policy(field))
                        return;
                    Backend::deserialize(field.value(), node, ctx);
                }
                catch (deserialize_error& ex)
                {
                    std::string msg = "error when deserializing field " + std::string(field.name());
                    ex.add(msg.c_str());
                    throw;
                }
            }
        });
    }
    else if (Backend::is_sequence(value))
    {
        constexpr auto max_sequence_size = sequence_deserializable_field_count<Reflectable>();
        if (max_sequence_size < Backend::size(value))
        {
            throw deserialize_error{"sequence size is greater than declared struct's field count"};
        }

        ctti::reflect_object(out, [&value, &ctx, index = 0U](auto field) mutable {
            check_field_attributes<decltype(field)>();

            if constexpr (!has_attribute<attributes::skip_deserialization_t>(field))
            {
                try
                {
                    if constexpr (decltype(field)::template has<attributes::extra_fields_t>())
                    {
                        throw deserialize_error{
                            "extra_fields fields are not supported in sequence deserialization"};
                    }
                    else if constexpr (decltype(field)::template has<attributes::flatten_t>())
                    {
                        throw deserialize_error{
                            "flatten fields are not supported in sequence deserialization"};
                    }

                    Backend::deserialize(field.value(), Backend::at_index(value, index), ctx);
                    ++index;
                }
                catch (deserialize_error& ex)
                {
                    std::string msg = "error when deserializing element " + std::to_string(index);
                    ex.add(msg.c_str());
                    throw;
                }
            }
        });
    }
    else
    {
        throw deserialize_error{"type must be a sequence or map but is a " +
                                Backend::type_name(value)};
    }
}
catch (deserialize_error& ex)
{
    std::string msg = "error when deserializing type " + std::string(ctti::name<Reflectable>());
    ex.add(msg.c_str());
    throw;
}

template <typename Backend, typename Enum, typename Context,
          enable_if<std::is_enum<Enum>> = true>
void deserialize_adl(Enum& out, const typename Backend::value_type& value, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
    {
        std::string text;
        Backend::deserialize(text, value, ctx);
        if (auto enum_value = kl::from_string<Enum>(text))
        {
            out = *enum_value;
            return;
        }

        throw deserialize_error{"invalid enum value: " + text};
    }
    else
    {
        std::underlying_type_t<Enum> underlying_value{};
        Backend::deserialize(underlying_value, value, ctx);
        out = static_cast<Enum>(underlying_value);
    }
}

template <typename Backend, typename Enum, typename Context>
void deserialize_adl(enum_set<Enum>& out, const typename Backend::value_type& value,
                     Context& ctx)
{
    Backend::expect_sequence(value);
    out = {};

    Backend::for_each_element(value, [&out, &ctx](const auto& item) {
        Enum e{};
        Backend::deserialize(e, item, ctx);
        out |= e;
    });
}

namespace impl {

template <typename Backend, typename Tuple, typename Context, std::size_t... Is>
void deserialize_tuple(Tuple& out, const typename Backend::value_type& value,
                       Context& ctx, std::index_sequence<Is...>)
{
    (Backend::deserialize(std::get<Is>(out), Backend::at_index(value, Is), ctx), ...);
}

} // namespace impl

template <typename Backend, typename Context, typename... Ts>
void deserialize_adl(std::tuple<Ts...>& out, const typename Backend::value_type& value,
                     Context& ctx)
{
    Backend::expect_sequence(value);
    if (sizeof...(Ts) < Backend::size(value))
        throw deserialize_error{"sequence size is greater than declared tuple field count"};
    impl::deserialize_tuple<Backend>(out, value, ctx,
                                     std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Backend, typename T, typename Context>
void deserialize_adl(std::optional<T>& out, const typename Backend::value_type& value,
                     Context& ctx)
{
    if (Backend::is_null(value))
        return out.reset();
    T element{};
    Backend::deserialize(element, value, ctx);
    out = std::move(element);
}

} // namespace kl::serialization::detail
