#pragma once

#include "kl/enum_traits.hpp"
#include "kl/utility.hpp"
#include "kl/iterator_facade.hpp"

#include <cstddef>
#include <type_traits>

namespace kl {

template <typename Enum>
class enum_iterator
    : public iterator_facade<enum_iterator<Enum>, Enum,
                             std::random_access_iterator_tag>
{
    using traits = enum_traits<Enum>;
    static_assert(traits::support_range, "Enum must support range enum trait");

public:
    constexpr enum_iterator() : index_{traits::max_value()} {}
    constexpr explicit enum_iterator(Enum value)
        : index_{underlying_cast(value)}
    {
    }

private:
    std::underlying_type_t<Enum> index_;

public:
    void advance(std::ptrdiff_t n) { index_ += n; }
    void decrement() { --index_; }
    void increment() { ++index_; }

    std::ptrdiff_t distance_to(const enum_iterator& other) const
    {
        return other.index_ - index_;
    }

    bool equal_to(const enum_iterator& other) const
    {
        return other.index_ == index_;
    }

    Enum dereference() const { return static_cast<Enum>(index_); }
};

template <typename Enum>
struct enum_range
{
    constexpr enum_iterator<Enum> begin() const
    {
        return enum_iterator<Enum>{enum_traits<Enum>::min()};
    }

    constexpr enum_iterator<Enum> end() const { return {}; }
};
} // namespace kl
