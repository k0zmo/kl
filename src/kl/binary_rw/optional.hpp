#pragma once

#include "kl/binary_rw.hpp"

#include <boost/optional.hpp>

namespace kl {

template <typename T>
kl::binary_reader& operator>>(kl::binary_reader& r, boost::optional<T>& opt)
{
    const auto not_empty = r.read<std::uint8_t>();
    if (!r.err() && not_empty)
        opt = r.read<T>();
    else
        opt = boost::none;

    return r;
}
} // namespace kl