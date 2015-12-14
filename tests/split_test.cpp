#include "kl/split.hpp"

#include <catch/catch.hpp>

TEST_CASE("split")
{
    SECTION("skip empty")
    {
        auto res = kl::split("ASD  QWE ZXC ");
        REQUIRE(res.size() == 3);
        REQUIRE(res[0] == "ASD");
        REQUIRE(res[1] == "QWE");
        REQUIRE(res[2] == "ZXC");
    }

    SECTION("dont skip empty")
    {
        auto res = kl::split("ASD  QWE ZXC ", " ", false);
        REQUIRE(res.size() == 4);
        REQUIRE(res[0] == "ASD");
        REQUIRE(res[1] == "");
        REQUIRE(res[2] == "QWE");
        REQUIRE(res[3] == "ZXC");
    }

    SECTION("pick delimeter")
    {
        auto res = kl::split("test1/test2", "/");
        REQUIRE(res.size() == 2);
        REQUIRE(res[0] == "test1");
        REQUIRE(res[1] == "test2");
    }

    SECTION("multiple delims")
    {
        auto res = kl::split("test1/test2\\test3\\test4.test5", "/\\.");
        REQUIRE(res.size() == 5);
        REQUIRE(res[0] == "test1");
        REQUIRE(res[1] == "test2");
        REQUIRE(res[2] == "test3");
        REQUIRE(res[3] == "test4");
        REQUIRE(res[4] == "test5");
    }

    SECTION("multiple delims with empties")
    {
        auto res = kl::split("test1//test2\\test3\\\\test4.test5", "/\\.");
        REQUIRE(res.size() == 5);
        REQUIRE(res[0] == "test1");
        REQUIRE(res[1] == "test2");
        REQUIRE(res[2] == "test3");
        REQUIRE(res[3] == "test4");
        REQUIRE(res[4] == "test5");
    }
}
