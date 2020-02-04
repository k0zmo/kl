#pragma once

#include "kl/byte.hpp"

#include <gsl/span>
#include <gsl/string_span>

#include <string>
#include <vector>
#include <optional>

namespace kl {

std::string base64_encode(gsl::span<const byte> s);
std::optional<std::vector<byte>> base64_decode(gsl::cstring_span<> str);
} // namespace kl
