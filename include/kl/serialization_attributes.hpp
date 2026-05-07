#pragma once

#include <cstddef>
#include <type_traits>

namespace kl::serialization {

struct skip_serialization_t {};
struct skip_deserialization_t {};
struct skip_t : skip_serialization_t, skip_deserialization_t {};

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

inline constexpr skip_serialization_t skip_serialization{};
inline constexpr skip_deserialization_t skip_deserialization{};
inline constexpr skip_t skip{};

template <typename... Names>
constexpr aliases_t aliases(Names... names)
{
    return aliases_t{names...};
}

} // namespace kl::serialization
