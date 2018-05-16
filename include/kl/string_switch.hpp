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
