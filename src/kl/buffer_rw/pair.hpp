#pragma once

#include "kl/buffer_rw.hpp"

#include <utility>

namespace kl {

template <typename T1, typename T2>
kl::buffer_reader& operator>>(kl::buffer_reader& r, std::pair<T1, T2>& pair)
{
    if (!r.err())
    {
        r >> pair.first;
        if (!r.err())
            r >> pair.second;
    }

    return r;
}
} // namespace kl
