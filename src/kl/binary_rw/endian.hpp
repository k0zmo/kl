#pragma once

#include "kl/binary_rw.hpp"

#include <boost/endian/arithmetic.hpp>

namespace kl {

template <boost::endian::order Order, typename T, std::size_t n_bits,
          boost::endian::align Align>
void write_binary(
    binary_writer& w,
    const boost::endian::endian_arithmetic<Order, T, n_bits, Align>& value)
{
    w.write_raw(value);
}

template <boost::endian::order Order, typename T, std::size_t n_bits,
          boost::endian::align Align>
void read_binary(
    binary_reader& r,
    boost::endian::endian_arithmetic<Order, T, n_bits, Align>& value)
{
    r.read_raw(value);
}
} // namespace kl
