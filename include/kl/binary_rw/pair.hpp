#pragma once

#include "kl/binary_rw.hpp"

#include <utility>

namespace kl {

template <typename T1, typename T2>
void write_binary(kl::binary_writer& w, const std::pair<T1, T2>& pair)
{
    w << pair.first;
    w << pair.second;
}

template <typename T1, typename T2>
void read_binary(kl::binary_reader& r, std::pair<T1, T2>& pair)
{
    if (!r.err())
    {
        r >> pair.first;
        if (!r.err())
            r >> pair.second;
    }
}
} // namespace kl
