#pragma once

#include "kl/binary_rw.hpp"
#include "kl/type_traits.hpp"

#include <vector>

namespace kl {

namespace detail {

// Specialized operator>> for vector<T> of trivial type T
template <typename T>
void decode_vector(kl::binary_reader& r, std::vector<T>& vec,
                   std::true_type /*is_trivial*/)
{
    const auto size = r.read<std::uint32_t>();

    vec.clear();

    if (size)
    {
        auto span = r.view(size * sizeof(T));
        if (r.err())
            return;

        vec.resize(size);
        // Use .data() so we get a pointer and memmove fast path
        std::copy_n(span.data(), size, vec.begin());
    }    
}

template <typename T>
void decode_vector(kl::binary_reader& r, std::vector<T>& vec,
                   std::false_type /*is_trivial*/)
{
    const auto size = r.read<std::uint32_t>();

    vec.clear();
    vec.reserve(size);

    for (std::uint32_t i = 0; i < size; ++i)
    {
        vec.push_back(r.read<T>());
        if (r.err())
        {
            vec.clear();
            break;
        }
    }
}
} // namespace detail

template <typename T>
kl::binary_reader& operator>>(kl::binary_reader& r, std::vector<T>& vec)
{

    detail::decode_vector(r, vec,
                          kl::bool_constant<std::is_trivial<T>::value>{});
    return r;
}
} // namespace kl
