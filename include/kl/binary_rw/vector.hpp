#pragma once

#include "kl/binary_rw.hpp"
#include "kl/type_traits.hpp"

#include <vector>

namespace kl {

namespace detail {

// Specialized operator<< for vector<T> of trivially copyable type T
template <typename T>
void encode_vector(kl::binary_writer& w, const std::vector<T>& vec,
                   std::true_type /*is_trivially_serializable*/)
{
    w << static_cast<std::uint32_t>(vec.size());
    w << gsl::span<const T>{vec};
}

template <typename T>
void encode_vector(kl::binary_writer& w, const std::vector<T>& vec,
                   std::false_type /*is_trivially_serializable*/)
{
    w << static_cast<std::uint32_t>(vec.size());

    for (const auto& item : vec)
        w << item;
}

// Specialized operator>> for vector<T> of trivial type T
template <typename T>
void decode_vector(kl::binary_reader& r, std::vector<T>& vec,
                   std::true_type /*is_trivially_deserializable*/)
{
    const auto size = r.read<std::uint32_t>();

    vec.clear();

    if (size)
    {
        vec.resize(size);
        r >> gsl::span<T>{vec};

        if (r.err())
        {
            vec.clear();
            return;
        }
    }
}

template <typename T>
void decode_vector(kl::binary_reader& r, std::vector<T>& vec,
                   std::false_type /*is_trivially_deserializable*/)
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

template <typename T>
using is_trivially_serializable = kl::bool_constant<is_simple<T>::value>;

template <typename T>
using is_trivially_deserializable = kl::bool_constant<is_simple<T>::value>;
} // namespace detail

template <typename T>
void write_binary(kl::binary_writer& w, const std::vector<T>& vec)
{
    detail::encode_vector(w, vec, detail::is_trivially_serializable<T>{});
}

template <typename T>
void read_binary(kl::binary_reader& r, std::vector<T>& vec)
{
    detail::decode_vector(r, vec, detail::is_trivially_deserializable<T>{});
}
} // namespace kl
