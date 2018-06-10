#include "kl/hash.hpp"

#include <catch/catch.hpp>
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
}
