#pragma once

#include "kl/binary_rw.hpp"

#include <vector>

namespace kl {

kl::binary_writer& operator<<(kl::binary_writer& w, const std::string& str)
{
    w << static_cast<std::uint32_t>(str.size());

    if (!str.empty())
        w << gsl::make_span(str);

    return w;
}

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
        // Use span::data() so we get a raw pointer and thus memmove fast path
        // NOTE: We can't r>>span like in vector because we don't have non-const
        // string::data() until C++17. 
        std::copy_n(span.data(), size, str.begin());
    }

    return r;
}
} // namespace kl
