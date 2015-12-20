#include "kl/match.hpp"

#include <catch/catch.hpp>

TEST_CASE("match")
{
    SECTION("overloader")
    {
        auto f = kl::make_overloader<float>([](int i) { return i * 2.0f; },
                                            [](float f) { return f * 2.5f; });
        REQUIRE(f(2) == 4.0f);
        REQUIRE(f(2.0f) == 5.0f);
    }

    SECTION("match")
    {
        boost::variant<int, float, bool> v = true;

        auto res = kl::match(v, [](int i) { return i * 2.0f; },
                                [](float f) { return f * 2.5f; },
                                [](bool b) { return b * 2.0f; });
        REQUIRE(res == 2.0f);
    }
}
