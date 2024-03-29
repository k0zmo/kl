#include "kl/defer.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("defer")
{
    int i = 0;
    {
        KL_DEFER(i = 1);
        i = 2;
    }
    REQUIRE(i == 1);
}
