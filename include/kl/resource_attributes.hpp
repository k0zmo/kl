#pragma once

namespace kl::resources::attributes {

struct child_key_base_t {};

template <auto Ptr>
struct child_key_t : child_key_base_t
{
    static constexpr auto ptr = Ptr;
};

// Marks the field as immutable (but not its descendants)
inline constexpr struct read_only_t {} read_only{};

// Marks all the child fields as immutable (but not the field itself)
inline constexpr struct subtree_read_only_t {} subtree_read_only{};

// Traverses a collection field by matching this member value instead of by index.
template <auto Ptr>
inline constexpr child_key_t<Ptr> child_key{};

} // namespace kl::resources::attributes
