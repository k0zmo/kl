#include "kl/hash.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <cstring>

TEST_CASE("hash")
{
    SECTION("constexpr hash")
    {
        constexpr auto h1 = kl::hash::fnv1a("test string");
        using namespace kl::hash::operators;
        constexpr auto h2 = "test string"_h;
        REQUIRE(h1 == h2);
        std::string str("test string");
        auto h3 = kl::hash::fnv1a(str);
        REQUIRE(h1 == h3);
    }

    SECTION("switch with a string literal")
    {
        using namespace kl::hash::operators;
        const char* hh = "3";

        switch (kl::hash::fnv1a(hh, strlen(hh)))
        {
        default:
            REQUIRE(false);
        case "3"_h:
            REQUIRE(true);
        }
    }

    SECTION("switch with a std::string")
    {
        using namespace kl::hash::operators;
        std::string s = "test";

        switch (kl::hash::fnv1a(s))
        {
        default:
            REQUIRE(false);
        case "test"_h:
            REQUIRE(true);
        }
    }

    SECTION("Hsieh's")
    {
        static_assert(kl::hash::hsieh("QWEASDZXC", 9) == 0xAEB8600C, "");
        REQUIRE(kl::hash::hsieh(nullptr, 0) == 0);
        REQUIRE(kl::hash::hsieh("QWEASDZXC", 9) == 0xAEB8600C);
        REQUIRE(kl::hash::hsieh("QWEASDZX", 8) == 0xE0B4386A);
        REQUIRE(kl::hash::hsieh("QWEASDZ", 7) == 0xD439CF4C);
        REQUIRE(kl::hash::hsieh("QWEASD", 6) == 0x79EF41CA);
    }
}
