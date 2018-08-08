#include "kl/utility.hpp"

#include <catch/catch.hpp>

TEST_CASE("utility")
{
    SECTION("countof")
    {
        int arr[3];
        std::array<bool, 4> stdarr;
        static_assert(kl::countof(arr) == 3, "");
        static_assert(kl::countof(stdarr) == 4, "");
        (void)arr;
        (void)stdarr;
    }

    SECTION("bitwise_cast")
    {
        float f = 3.14f;
        REQUIRE(kl::bitwise_cast<uint32_t>(f) == 0x4048f5c3);
        REQUIRE(kl::bitwise_cast<float>(0x4048f5c3U) == 3.14f);
    }
}
