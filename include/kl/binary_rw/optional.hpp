#pragma once

#include "kl/binary_rw.hpp"

#include <optional>

namespace kl {

template <typename T>
void write_binary(kl::binary_writer& w, const std::optional<T>& opt)
{
    w << static_cast<std::uint8_t>(!!opt);
    if (opt)
        w << *opt;
}

template <typename T>
void read_binary(kl::binary_reader& r, std::optional<T>& opt)
{
    const auto not_empty = r.read<std::uint8_t>();
    if (!r.err() && not_empty)
        opt = r.read<T>();
    else
        opt = std::nullopt;
}
} // namespace kl
