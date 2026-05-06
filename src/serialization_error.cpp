#include "kl/serialization_error.hpp"

#include <iterator>

namespace kl::serialization {

void deserialize_error::add(const char* message)
{
    messages_.insert(end(messages_), '\n');
    messages_.append(message);
}

deserialize_error::~deserialize_error() noexcept = default;
parse_error::~parse_error() noexcept = default;

} // namespace kl::serialization
