#pragma once

#include "kl/byte.hpp"

#include <gsl/span>
#include <gsl/string_span>

#include <memory>

namespace kl {

class file_view
{
public:
    explicit file_view(const char* file_path);
    ~file_view();

    gsl::span<const byte> get_bytes() const { return contents_; }

private:
    gsl::span<const byte> contents_;
};
} // namespace kl
