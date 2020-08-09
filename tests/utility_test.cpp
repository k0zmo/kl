#include "kl/utility.hpp"

#include <catch2/catch.hpp>

TEST_CASE("bit_cast")
{
    float f = 3.14f;
    REQUIRE(kl::bit_cast<uint32_t>(f) == 0x4048f5c3);
    REQUIRE(kl::bit_cast<float>(0x4048f5c3U) == 3.14f);
}
