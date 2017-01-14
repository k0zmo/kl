#pragma once

#include "kl/buffer_rw.hpp"

#include <set>

namespace kl {

template <typename T>
kl::buffer_reader& operator>>(kl::buffer_reader& r, std::set<T>& set)
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
