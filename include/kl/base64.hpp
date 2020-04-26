#pragma once

#include <gsl/span>

#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <string_view>

namespace kl {

std::string base64_encode(gsl::span<const std::byte> s);
std::optional<std::vector<std::byte>> base64_decode(std::string_view str);
} // namespace kl
