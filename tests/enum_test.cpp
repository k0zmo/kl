#include "kl/enum_traits.hpp"
#include "kl/enum_range.hpp"

#include <catch/catch.hpp>
#include <string>

namespace ns {
namespace inner {
enum class colour_space
{
    rgb,
    xyz,
    ycrcb,
    hsv,
    lab,
    hls,
    luv
};
} // namespace inner
} // namespace ns

enum class access_mode
{
    read_write,
    write_only,
    read_only,
    max
};

namespace kl {
using namespace ns::inner;
template <>
struct enum_traits<colour_space>
    : enum_trait_support_range<colour_space, colour_space::rgb,
                               colour_space::luv, false>
{
};

template <>
struct enum_traits<access_mode>
    : enum_trait_support_range<access_mode, access_mode::read_write,
                               access_mode::max>
{
};
} // namespace kl

TEST_CASE("enum_range")
{
    SECTION("access_mode with ::max")
    {
        std::vector<access_mode> vec;

        for (const auto v : kl::enum_range<access_mode>())
        {
            vec.push_back(v);
        }

        REQUIRE(vec.size() == 3);
        REQUIRE(vec[0] == access_mode::read_write);
        REQUIRE(vec[1] == access_mode::write_only);
        REQUIRE(vec[2] == access_mode::read_only);
    }

    SECTION("colour_space without ::max")
    {
        using ns::inner::colour_space;

        std::vector<colour_space> vec;

        for (const auto v : kl::enum_range<colour_space>())
        {
            vec.push_back(v);
        }

        REQUIRE(vec.size() == 7);
        REQUIRE(vec[0] == colour_space::rgb);
        REQUIRE(vec[1] == colour_space::xyz);
        REQUIRE(vec[2] == colour_space::ycrcb);
        REQUIRE(vec[3] == colour_space::hsv);
        REQUIRE(vec[4] == colour_space::lab);
        REQUIRE(vec[5] == colour_space::hls);
        REQUIRE(vec[6] == colour_space::luv);
    }
}
