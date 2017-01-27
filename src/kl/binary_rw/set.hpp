#pragma once

#include "kl/binary_rw.hpp"

#include <set>

namespace kl {

template <typename T>
kl::binary_writer& operator<<(kl::binary_writer& w, const std::set<T>& set)
{
    w << static_cast<std::uint32_t>(set.size());

    for (const auto& key : set)
        w << key;

    return w;
}

template <typename T>
kl::binary_reader& operator>>(kl::binary_reader& r, std::set<T>& set)
{
    const auto size = r.read<std::uint32_t>();
    set.clear();

    for (std::uint32_t i = 0; i < size; ++i)
    {
        set.insert(r.read<T>());
        if (r.err())
        {
            set.clear();
            break;
        }
    }

    return r;
}
} // namespace kl
