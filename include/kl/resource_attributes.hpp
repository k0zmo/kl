#pragma once

#include <type_traits>

namespace kl::resources::attributes {

struct read_only_t {};
struct subtree_read_only_t {};
struct child_key_base_t {};

template <auto Ptr>
struct child_key_t : child_key_base_t
{
    static constexpr auto ptr = Ptr;
};

namespace detail {

template <typename T>
struct is_child_key : std::false_type {};

template <auto Ptr>
struct is_child_key<child_key_t<Ptr>> : std::true_type {};

template <typename T>
inline constexpr bool is_child_key_v = is_child_key<T>::value;

} // namespace detail

// Marks the field as immutable (but not its descendants)
inline constexpr read_only_t read_only{};

// Marks all the child fields as immutable (but not the field itself)
inline constexpr subtree_read_only_t subtree_read_only{};

// Traverses a collection field by matching this member value instead of by index.
template <auto Ptr>
inline constexpr child_key_t<Ptr> child_key{};

} // namespace kl::resources::attributes
