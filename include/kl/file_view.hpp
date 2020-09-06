#pragma once

#include <gsl/span>

#include <cstddef>

namespace kl {

class file_view
{
public:
    explicit file_view(const char* file_path);
    ~file_view();

    gsl::span<const std::byte> get_bytes() const noexcept { return contents_; }

private:
    gsl::span<const std::byte> contents_;
};
} // namespace kl
