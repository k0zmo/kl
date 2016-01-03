#pragma once

#include <type_traits>
#include <utility>

namespace kl {

// Casts any kind of enum (enum, enum class) to its underlying type (i.e int32_t)
template <typename Enum>
constexpr auto underlying_cast(Enum e)
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

// Iteratates over continuous range [first, last) of given enum and execute a
// unary function over it
template <typename Enum, typename Func>
void for_each_enum(Enum first, Enum last, Func&& func)
{
    for (auto i = underlying_cast(first); i < underlying_cast(last); ++i)
        std::forward<Func>(func)(static_cast<Enum>(i));
}

template <typename Enum>
struct enum_traits
{
    static constexpr bool does_index = false;
    static constexpr bool enable_bitmask = false;
    static constexpr bool enable_bitshift = false;
    static constexpr bool support_range = false;
};

// Specialize enum_traits template class deriving this struct to enable indexing
// for given enum type
struct enum_trait_support_indexing
{
    static constexpr bool does_index = true;
};

// Specialize enum_traits template class deriving this struct to enable
// bitmasking operators for given enum type
struct enum_trait_support_bitmask
{
    static constexpr bool enable_bitmask = true;
};

// Specialize enum_traits template class deriving this struct to enable
// bitshifting operators for given enum type
struct enum_trait_support_bitshift
{
    static constexpr bool enable_bitshift = true;
};

// Specialize enum_traits template class deriving this to enable querying for
// valid range (open, closed interval) for given enum type
template <typename Enum, Enum Min, Enum Max, bool open_closed = true>
struct enum_trait_support_range
{
    static constexpr bool support_range = true;

    using underlying_type = std::underlying_type_t<Enum>;

    static constexpr underlying_type min_value()
    {
        return underlying_cast(Min);
    }
    static constexpr underlying_type max_value()
    {
        static_assert(open_closed ||
                          (underlying_cast(Max) + 1 > underlying_cast(Max)),
                      "overflow of enum range");
        return open_closed ? underlying_cast(Max) : underlying_cast(Max) + 1;
    }

    static constexpr Enum min() { return static_cast<Enum>(min_value()); };
    static constexpr Enum max() { return static_cast<Enum>(max_value()); }
    static constexpr std::size_t count() { return max_value() - min_value(); }

    static_assert(count() > 0, "count() > 0 constaint is not fulfilled");

    static bool in_range(Enum e)
    {
        const auto v = underlying_cast(e);
        return v >= min_value() && v < max_value();
    }

    template <typename Integral,
              typename = std::enable_if_t<std::is_integral<Integral>::value>>
    static bool in_range(Integral v)
    {
        return v >= min_value() && v < max_value();
    }
};

/*
* Arithmetic operators defined for each enum type that support
* enum_trait_support_indexing trait
*/
template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::does_index>>
constexpr auto operator+(Enum e)
{
    return underlying_cast(e);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::does_index>>
constexpr Enum& operator++(Enum& e)
{
    return e = static_cast<Enum>(+e + 1);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::does_index>>
constexpr Enum operator++(Enum& e, int)
{
    // postfix
    const auto ret = e;
    e = static_cast<Enum>(+e + 1);
    return ret;
}

namespace enums {
namespace operators {
template <typename Enum>
constexpr Enum operator|(Enum left, Enum right)
{
    return static_cast<Enum>(underlying_cast(left) | underlying_cast(right));
}

template <typename Enum>
constexpr Enum& operator|=(Enum& left, Enum right)
{
    return left = enums::operators::operator|(left, right);
}

template <typename Enum>
constexpr Enum operator&(Enum left, Enum right)
{
    return static_cast<Enum>(underlying_cast(left) & underlying_cast(right));
}

template <typename Enum>
constexpr Enum& operator&=(Enum& left, Enum right)
{
    return left = enums::operators::operator&(left, right);
}

template <typename Enum>
constexpr Enum operator^(Enum left, Enum right)
{
    return static_cast<Enum>(underlying_cast(left) ^ underlying_cast(right));
}

template <typename Enum>
constexpr Enum& operator^=(Enum& left, Enum right)
{
    return left = enums::operators::operator^(left, right);
}

template <typename Enum>
constexpr Enum operator~(Enum e)
{
    return static_cast<Enum>(~underlying_cast(e));
}
} // namespace operators
} // namespace enums

/*
 * Binary arithmetic operators defined for each enum type that support
 * enum_trait_support_bitmask trait
 */
template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum operator|(Enum left, Enum right)
{
    return enums::operators::operator|(left, right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum operator&(Enum left, Enum right)
{
    return enums::operators::operator&(left, right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum operator^(Enum left, Enum right)
{
    return enums::operators::operator^(left, right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum& operator|=(Enum& left, Enum right)
{
    return enums::operators::operator|=(left, right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum& operator&=(Enum& left, Enum right)
{
    return enums::operators::operator&=(left, right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum& operator^=(Enum& left, Enum right)
{
    return enums::operators::operator^=(left, right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr Enum operator~(Enum e)
{
    return enums::operators::operator~(e);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitmask>>
constexpr bool check_mask(Enum v, Enum mask)
{
    return (underlying_cast(v) & underlying_cast(mask)) != 0;
}

/*
* Binary arithmetic (shifting) operators defined for each enum type that support
* enum_trait_support_bitshift trait
*/
template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitshift>>
constexpr Enum operator<<(Enum left, int right)
{
    return static_cast<Enum>(underlying_cast(left) << right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitshift>>
constexpr Enum operator>>(Enum left, int right)
{
    return static_cast<Enum>(underlying_cast(left) >> right);
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitshift>>
constexpr Enum& operator<<=(Enum& left, int right)
{
    left = static_cast<Enum>(underlying_cast(left) << right);
    return left;
}

template <typename Enum,
          typename = std::enable_if_t<enum_traits<Enum>::enable_bitshift>>
constexpr Enum& operator>>=(Enum& left, int right)
{
    left = static_cast<Enum>(underlying_cast(left) >> right);
    return left;
}
} // namespace kl
