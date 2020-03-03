#pragma once

#include <gsl/span>
#include <gsl/string_span>

#include <cstddef>
#include <string>
#include <vector>
#include <optional>

namespace kl {

std::string base64_encode(gsl::span<const std::byte> s);
std::optional<std::vector<std::byte>> base64_decode(gsl::cstring_span<> str);
} // namespace kl
