#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace kl {
namespace detail {

#if __cplusplus == 201103 || (defined(_MSC_VER) && _MSC_VER == 1900)

constexpr std::uint32_t fnv1a_impl(std::uint32_t accu, std::uint8_t data)
{
    return static_cast<uint32_t>((accu ^ static_cast<std::uint32_t>(data)) *
                                 16777619U);
}

constexpr std::uint32_t fnv1a(std::uint32_t accu, const char* data,
                              std::size_t length)
{
    return length > 0 ? fnv1a(fnv1a_impl(accu, data[0]), data + 1, length - 1)
                      : accu;
}

constexpr std::uint32_t fnv1a(const char* data, std::size_t length)
{
    return fnv1a(2166136261U, data, length);
}

#elif __cplusplus >= 201402 || (defined(_MSC_VER) && _MSC_VER >= 1910)

constexpr uint32_t fnv1a(const char* data, std::size_t length)
{
    uint32_t hash = 2166136261U;
    for (std::size_t i = 0; i < length; ++i)
        hash = (hash ^ static_cast<uint32_t>(data[i])) * 16777619U;
    return hash;
}

#else
static_assert(false, "Required C++11 constexpr");
#endif
} // namespace detail

namespace hash {

constexpr std::uint32_t fnv1a(const char* data, std::size_t length)
{
    return kl::detail::fnv1a(data, length);
}

template <std::size_t N>
constexpr std::uint32_t fnv1a(const char (&str)[N])
{
    // Get rid of null
    return str[N - 1] != '\0' ? throw std::logic_error{""}
                              : kl::detail::fnv1a(str, N - 1);
}

inline uint32_t fnv1a(const std::string& str)
{
    return fnv1a(str.c_str(), str.size());
}

namespace operators {
constexpr uint32_t operator""_h(const char* data, size_t length)
{
    return fnv1a(data, length);
}
} // namespace operators

namespace detail {

constexpr std::uint16_t get16bits(const char* d)
{
    return (d[0] | (d[1] << 8));
}
} // namespace detail

#if defined(_MSC_VER) && _MSC_VER < 1910
#define constexpr14
#else
#define constexpr14 constexpr
#endif

// From http://www.azillionmonkeys.com/qed/hash.html
constexpr14 std::uint32_t hsieh(const char* data, std::size_t length)
{
    auto hash = static_cast<uint32_t>(length);

    if (length <= 0 || data == nullptr)
        return 0;

    int rem = length & 3;
    length >>= 2;

    /* Main loop */
    for (; length > 0; length--)
    {
        hash += detail::get16bits(data);
        uint32_t tmp = (detail::get16bits(data + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        data += 2 * sizeof(uint16_t);
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch (rem)
    {
    case 3:
        hash += detail::get16bits(data);
        hash ^= hash << 16;
        hash ^= ((signed char)data[sizeof(uint16_t)]) << 18;
        hash += hash >> 11;
        break;
    case 2:
        hash += detail::get16bits(data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1:
        hash += (signed char)*data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

#undef constexpr14

} // namespace hash
} // namespace kl

/*
    std::string test = ...

    using namespace kl::hash::operators;

    switch (kl::hash::fnv1a(test))
    {
    case "1"_h:
        // ...
        break;
    case "2"_h:
        // ...
        break;
    case "3"_h:
        // ...
        break;
    default:
        // ...
        break;
    }
*/
