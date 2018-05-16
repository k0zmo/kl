#pragma once

#include "kl/byte.hpp"

#include <gsl/span>
#include <gsl/string_span>
#include <boost/optional.hpp>

#include <string>
#include <vector>

namespace kl {

std::string base64_encode(gsl::span<const byte> s);
boost::optional<std::vector<byte>> base64_decode(gsl::cstring_span<> str);
} // namespace kl
