#pragma once

namespace kl::serialization {

struct skip_serialization_t {};
struct skip_deserialization_t {};
struct skip_t : skip_serialization_t, skip_deserialization_t {};

inline constexpr skip_serialization_t skip_serialization{};
inline constexpr skip_deserialization_t skip_deserialization{};
inline constexpr skip_t skip{};

} // namespace kl::serialization
