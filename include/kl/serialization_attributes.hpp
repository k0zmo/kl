#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace kl::serialization::attributes {

struct skip_serialization_t {};
struct skip_deserialization_t {};
struct skip_t : skip_serialization_t, skip_deserialization_t {};
struct skip_if_null_t {};
struct emit_null_t {};
struct allow_missing_t {};
struct skip_if_empty_t {};

template <typename T>
struct default_value_t
{
    T value;
};

// This is needed so we can query attributes by 'shape' (is it default_value<T>?)
// instead of requiring exact type match
namespace detail {

template <typename T>
struct is_default_value : std::false_type {};

template <typename T>
struct is_default_value<default_value_t<T>> : std::true_type
{
    using value_type = T;
};

template <typename T>
inline constexpr bool is_default_value_v = is_default_value<T>::value;
} // namespace detail

struct aliases_t
{
    static constexpr std::size_t max_aliases = 8;

    template <typename... Names>
    constexpr explicit aliases_t(Names... names)
        : size_{sizeof...(Names)}, names_{static_cast<const char*>(names)...}
    {
        static_assert(sizeof...(Names) <= max_aliases,
                      "too many serialization aliases");
        static_assert((std::is_convertible_v<Names, const char*> && ...),
                      "serialization aliases must be string literals or const char*");
    }

    constexpr auto begin() const noexcept { return names_; }
    constexpr auto end() const noexcept { return names_ + size_; }

private:
    std::size_t size_;
    const char* names_[max_aliases];
};

struct rename_t
{
    const char* name;
};

inline constexpr skip_serialization_t skip_serialization{};
inline constexpr skip_deserialization_t skip_deserialization{};
inline constexpr skip_t skip{};

// Serialization only. Omit this field when serialization::is_null_value(field.value()) is true.
// Overrides the context-wide skip_null_fields setting.
inline constexpr skip_if_null_t skip_if_null{};

// Serialization only. Force this field to be emitted even
// when the context-wide skip_null_fields is true.
inline constexpr emit_null_t emit_null{};

// Deserialization only. If the input key is absent, leave the existing field value unchanged.
inline constexpr allow_missing_t allow_missing{};

// Serialization only. Omit empty strings/containers
inline constexpr skip_if_empty_t skip_if_empty{};

// Deserialization only. If the input field is missing or present-but-null, assign value
template <typename T>
constexpr default_value_t<std::decay_t<T>> default_value(T&& value)
{
    return {std::forward<T>(value)};
}

template <typename... Names>
constexpr aliases_t aliases(Names... names)
{
    return aliases_t{names...};
}

constexpr rename_t rename(const char* name)
{
    return {name};
}

} // namespace kl::serialization::attributes
