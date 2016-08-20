#include "kl/index_sequence.hpp"

#include <catch/catch.hpp>

TEST_CASE("index_sequence")
{
    static_assert(std::is_same<kl::make_index_sequence<4>,
                               std::index_sequence<0, 1, 2, 3>>::value,
                  "???");
}
