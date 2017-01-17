#pragma once

#include "kl/binary_rw.hpp"

#include <vector>

namespace kl {

kl::binary_reader& operator>>(kl::binary_reader& r, std::string& str)
{
    const auto size = r.read<std::uint32_t>();
    str.clear();

    if (size)
    {
        auto span = r.view(size);
        if (r.err())
            return r;
        str.resize(size);
        // Use .data() so we get a pointer and memmove fast path
        std::copy_n(span.data(), size, str.begin());
    }

    return r;
}
} // namespace kl
