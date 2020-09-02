#pragma once

#include "kl/binary_rw.hpp"

#include <gsl/span_ext>

#include <vector>

namespace kl {

void write_binary(kl::binary_writer& w, const std::string& str)
{
    w << static_cast<std::uint32_t>(str.size());

    if (!str.empty())
        w << gsl::span<const char>{str};
}

void read_binary(kl::binary_reader& r, std::string& str)
{
    const auto size = r.read<std::uint32_t>();
    str.clear();

    if (!size)
        return;

    str.resize(size);
    r >> gsl::make_span(str);

    if (r.err())
        str.clear();
}
} // namespace kl
