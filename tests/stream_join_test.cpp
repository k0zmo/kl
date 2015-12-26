#include "kl/stream_join.hpp"

#include <catch/catch.hpp>
#include <vector>
#include <sstream>

TEST_CASE("stream_join")
{
    std::vector<int> v = {5, 4, 3, 2, 1};
    std::stringstream ss;

    SECTION("empty sequence")
    {
        v.clear();
        ss << kl::stream_join(v);
        REQUIRE(ss.str() == ".");
        ss.str("");
        ss << kl::stream_join(v).set_empty_string("[]");
        REQUIRE(ss.str() == "[]");
    }

    SECTION("default joiner")
    {
        ss << kl::stream_join(v);
        REQUIRE(ss.str() == "5, 4, 3, 2, 1");
    }

    SECTION("change delim")
    {
        ss << kl::stream_join(v).set_delimiter("|");
        REQUIRE(ss.str() == "5|4|3|2|1");
    }
}

TEST_CASE("ostream_joiner")
{
    std::stringstream ss;
    auto joiner = kl::make_ostream_joiner(ss, ", ");

    SECTION("manual assignment")
    {
        for (int i = 0; i < 5; ++i)
            joiner = i;
        REQUIRE(ss.str() == "0, 1, 2, 3, 4");
    }

    SECTION("copy from vector")
    {
        std::vector<int> v = {5, 4, 3, 2, 1};
        std::copy(v.begin(), v.end(), joiner);
        REQUIRE(ss.str() == "5, 4, 3, 2, 1");
    }
}
