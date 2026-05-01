#pragma once

#include "kl/detail/concepts.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/enum_set.hpp"
#include "kl/utility.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kl::serialization::detail {

// encode implementation

template <typename Backend, typename Map, typename Context,
          enable_if<::kl::detail::is_map_alike<Map>> = true>
void encode(const Map& map, Context& ctx)
{
    static_assert(std::is_constructible_v<std::string, typename Map::key_type>,
                  "std::string must be constructible from the Map's key type");

    Backend::begin_map(ctx);
    for (const auto& [key, value] : map)
    {
        if (!ctx.skip_field(key, value))
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
void encode(const Range& rng, Context& ctx)
{
    Backend::begin_sequence(ctx);
    for (const auto& value : rng)
        Backend::dump(value, ctx);
    Backend::end_sequence(ctx);
}

template <typename Backend, typename Reflectable, typename Context,
          enable_if<is_reflectable<Reflectable>> = true>
void encode(const Reflectable& refl, Context& ctx)
{
    Backend::begin_map(ctx);
    ctti::reflect(refl, [&ctx](auto& field, auto name) {
        if (!ctx.skip_field(name, field))
        {
            Backend::write_key(name, ctx);
            Backend::dump(field, ctx);
        }
    });
    Backend::end_map(ctx);
}

template <typename Backend, typename Enum, typename Context,
          enable_if<std::is_enum<Enum>> = true>
void encode(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        Backend::dump(kl::to_string(e), ctx);
    else
        Backend::dump(underlying_cast(e), ctx);
}

template <typename Backend, typename Enum, typename Context>
void encode(const enum_set<Enum>& set, Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");

    Backend::begin_sequence(ctx);
    for (const auto possible_value : reflect<Enum>().values())
    {
        if (set.test(possible_value))
            Backend::dump(kl::to_string(possible_value), ctx);
    }
    Backend::end_sequence(ctx);
}

namespace impl {

template <typename Backend, typename Tuple, typename Context, std::size_t... Is>
void encode_tuple(const Tuple& tuple, Context& ctx,
                  std::index_sequence<Is...>)
{
    Backend::begin_sequence(ctx);
    (Backend::dump(std::get<Is>(tuple), ctx), ...);
    Backend::end_sequence(ctx);
}

} // namespace impl

template <typename Backend, typename... Ts, typename Context>
void encode(const std::tuple<Ts...>& tuple, Context& ctx)
{
    impl::encode_tuple<Backend>(tuple, ctx,
                                std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Backend, typename T, typename Context>
void encode(const std::optional<T>& opt, Context& ctx)
{
    if (!opt)
        Backend::dump(nullptr, ctx);
    else
        Backend::dump(*opt, ctx);
}

// serialize implementation

template <typename Backend, typename Map, typename Context>
typename Backend::value_type serialize_map(const Map& map, Context& ctx)
{
    static_assert(std::is_constructible_v<std::string, typename Map::key_type>,
                  "std::string must be constructible from the Map's key type");

    auto out = Backend::make_map();
    for (const auto& [key, value] : map)
    {
        if (!ctx.skip_field(key, value))
            Backend::add_field(out, key, Backend::serialize(value, ctx), ctx);
    }
    return out;
}

template <typename Backend, typename Range, typename Context>
typename Backend::value_type serialize_range(const Range& rng, Context& ctx)
{
    auto out = Backend::make_sequence();
    for (const auto& value : rng)
        Backend::add_element(out, Backend::serialize(value, ctx), ctx);
    return out;
}

template <typename Backend, typename Reflectable, typename Context>
typename Backend::value_type serialize_reflectable(const Reflectable& refl,
                                                   Context& ctx)
{
    auto out = Backend::make_map();
    ctti::reflect(refl, [&out, &ctx](auto& field, auto name) {
        if (!ctx.skip_field(name, field))
            Backend::add_field(out, name, Backend::serialize(field, ctx), ctx);
    });
    return out;
}

template <typename Backend, typename Enum, typename Context>
typename Backend::value_type serialize_enum(Enum e, Context& ctx)
{
    if constexpr (is_enum_reflectable_v<Enum>)
        return Backend::serialize(kl::to_string(e), ctx);
    else
        return Backend::serialize(underlying_cast(e), ctx);
}

template <typename Backend, typename Enum, typename Context>
typename Backend::value_type serialize_enum_set(const enum_set<Enum>& set,
                                                Context& ctx)
{
    static_assert(is_enum_reflectable_v<Enum>,
                  "Only sets of reflectable enums are supported");

    auto out = Backend::make_sequence();
    for (const auto possible_value : reflect<Enum>().values())
    {
        if (set.test(possible_value))
        {
            Backend::add_element(out, Backend::serialize(possible_value, ctx),
                                 ctx);
        }
    }
    return out;
}

namespace impl {

template <typename Backend, typename Tuple, typename Context, std::size_t... Is>
typename Backend::value_type serialize_tuple_impl(const Tuple& tuple,
                                                  Context& ctx,
                                                  std::index_sequence<Is...>)
{
    auto out = Backend::make_sequence();
    (Backend::add_element(out, Backend::serialize(std::get<Is>(tuple), ctx),
                          ctx),
     ...);
    return out;
}

} // namespace impl

template <typename Backend, typename... Ts, typename Context>
typename Backend::value_type serialize_tuple(const std::tuple<Ts...>& tuple, Context& ctx)
{
    return impl::serialize_tuple_impl<Backend>(tuple, ctx,
                                               std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Backend, typename T, typename Context>
typename Backend::value_type serialize_optional(const std::optional<T>& opt,
                                                Context& ctx)
{
    if (opt)
        return Backend::serialize(*opt, ctx);
    return typename Backend::value_type{};
}

template <typename Backend, typename Map>
void deserialize_map(Map& out, const typename Backend::value_type& value)
{
    Backend::expect_map(value);

    out.clear();

    Backend::for_each_field(value, [&out](const auto& key, const auto& field) {
        try
        {
            typename Map::key_type key_value{};
            Backend::deserialize(key_value, key);
            typename Map::mapped_type mapped_value{};
            Backend::deserialize(mapped_value, field);
            out.emplace(std::move(key_value), std::move(mapped_value));
        }
        catch (typename Backend::deserialize_error& ex)
        {
            std::string field_name;
            Backend::deserialize(field_name, key);
            std::string msg =
                "error when deserializing field " + field_name;
            ex.add(msg.c_str());
            throw;
        }
    });
}

template <typename Backend, typename GrowableRange>
void deserialize_range(GrowableRange& out,
                       const typename Backend::value_type& value)
{
    Backend::expect_sequence(value);

    out.clear();
    if constexpr (::kl::detail::has_reserve_v<GrowableRange>)
        out.reserve(Backend::size(value));

    Backend::for_each_element(value, [&out](const auto& item) {
        try
        {
            typename GrowableRange::value_type element{};
            Backend::deserialize(element, item);
            out.push_back(std::move(element));
        }
        catch (typename Backend::deserialize_error& ex)
        {
            std::string msg = "error when deserializing element " +
                              std::to_string(out.size());
            ex.add(msg.c_str());
            throw;
        }
    });
}

template <typename Backend, typename Reflectable>
void deserialize_reflectable(Reflectable& out,
                             const typename Backend::value_type& value)
try
{
    if (Backend::is_map(value))
    {
        ctti::reflect(out, [&value](auto& field, auto name) {
            try
            {
                Backend::deserialize(field, Backend::at_field(value, name));
            }
            catch (typename Backend::deserialize_error& ex)
            {
                std::string msg =
                    "error when deserializing field " + std::string(name);
                ex.add(msg.c_str());
                throw;
            }
        });
    }
    else if (Backend::is_sequence(value))
    {
        if (Backend::size(value) > ctti::num_fields<Reflectable>())
        {
            throw typename Backend::deserialize_error{
                "sequence size is greater than declared struct's field count"};
        }

        ctti::reflect(out, [&value, index = 0U](auto& field, auto) mutable {
            try
            {
                Backend::deserialize(field, Backend::at_index(value, index));
                ++index;
            }
            catch (typename Backend::deserialize_error& ex)
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
        throw typename Backend::deserialize_error{
            "type must be a sequence or map but is a " +
            Backend::type_name(value)};
    }
}
catch (typename Backend::deserialize_error& ex)
{
    std::string msg = "error when deserializing type " +
                      std::string(ctti::name<Reflectable>());
    ex.add(msg.c_str());
    throw;
}

template <typename Backend, typename Enum>
void deserialize_enum(Enum& out, const typename Backend::value_type& value)
{
    if constexpr (is_enum_reflectable_v<Enum>)
    {
        std::string text;
        Backend::deserialize(text, value);
        if (auto enum_value = kl::from_string<Enum>(text))
        {
            out = *enum_value;
            return;
        }

        throw
            typename Backend::deserialize_error{"invalid enum value: " + text};
    }
    else
    {
        std::underlying_type_t<Enum> underlying_value{};
        Backend::deserialize(underlying_value, value);
        out = static_cast<Enum>(underlying_value);
    }
}

template <typename Backend, typename Enum>
void deserialize_enum_set(enum_set<Enum>& out,
                          const typename Backend::value_type& value)
{
    Backend::expect_sequence(value);
    out = {};

    Backend::for_each_element(value, [&out](const auto& item) {
        Enum e{};
        Backend::deserialize(e, item);
        out |= e;
    });
}

template <typename Backend, typename Tuple, std::size_t... Is>
void deserialize_tuple_impl(Tuple& out,
                            const typename Backend::value_type& value,
                            std::index_sequence<Is...>)
{
    (Backend::deserialize(std::get<Is>(out), Backend::at_index(value, Is)),
     ...);
}

template <typename Backend, typename... Ts>
void deserialize_tuple(std::tuple<Ts...>& out,
                       const typename Backend::value_type& value)
{
    Backend::expect_sequence(value);
    deserialize_tuple_impl<Backend>(out, value,
                                    std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Backend, typename T>
void deserialize_optional(std::optional<T>& out,
                          const typename Backend::value_type& value)
{
    if (Backend::is_null(value))
        return out.reset();
    T element{};
    Backend::deserialize(element, value);
    out = std::move(element);
}

} // namespace kl::serialization::detail
