#include "kl/enum_traits.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

namespace ns {
namespace inner {
enum class colour_space
{
    rgb = 2,
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

enum unscoped_enum_type { prefix_one, prefix_two };
enum class non_reflectable { one, two, three };
} // namespace

namespace kl {
using namespace ns::inner;
template <>
struct enum_traits<colour_space>
    : enum_trait_support_range<colour_space, colour_space::rgb,
                               colour_space::luv, false>,
      enum_trait_support_indexing
{
};

template <>
struct enum_traits<access_mode>
    : enum_trait_support_range<access_mode, access_mode::read_write,
                               access_mode::max>
{
};
} // namespace kl

enum class eee { a, b, c };

namespace kl {
template <>
struct enum_traits<eee>
    : enum_trait_support_indexing
{
};
} // namespace kl

TEST_CASE("enum_traits")
{
    static_assert(kl::enum_traits<access_mode>::min_value() == 0, "");
    static_assert(kl::enum_traits<access_mode>::max_value() == 2 + 1, "");
    static_assert(
        kl::enum_traits<access_mode>::min() == access_mode::read_write, "");
    static_assert(kl::enum_traits<access_mode>::max() == access_mode::max, "");
    static_assert(kl::enum_traits<access_mode>::count() == 3, "");

    static_assert(
        kl::enum_traits<access_mode>::in_range(access_mode::read_only), "");
    static_assert(!kl::enum_traits<access_mode>::in_range(access_mode::max),
                  "");
    static_assert(!kl::enum_traits<access_mode>::in_range(34), "");
    static_assert(kl::enum_traits<access_mode>::in_range(2), "");

    using namespace ns::inner;
    static_assert(kl::enum_traits<colour_space>::min_value() == 2, "");
    static_assert(kl::enum_traits<colour_space>::max_value() == 8 + 1, "");
    static_assert(kl::enum_traits<colour_space>::min() == colour_space::rgb,
                  "");
    static_assert(kl::enum_traits<colour_space>::max() ==
                      static_cast<colour_space>(+colour_space::luv + 1),
                  "");
    static_assert(kl::enum_traits<colour_space>::count() == 7, "");
    static_assert(kl::enum_traits<colour_space>::in_range(colour_space::luv),
                  "");
    static_assert(
        !kl::enum_traits<colour_space>::in_range(static_cast<colour_space>(11)),
        "");
    static_assert(!kl::enum_traits<colour_space>::in_range(1), "");
    static_assert(kl::enum_traits<colour_space>::in_range(2), "");

    SECTION("enum operators")
    {
        REQUIRE(+colour_space::lab == kl::underlying_cast(colour_space::lab));

        auto e = colour_space::lab;
        auto e1 = ++e;
        REQUIRE(e1 == colour_space::hls);
        REQUIRE(e == colour_space::hls);

        auto e2 = e++;
        REQUIRE(e2 == colour_space::hls);
        REQUIRE(e == colour_space::luv);

        {
            using namespace kl::enums::operators;
            REQUIRE(+access_mode::max == 3);

            auto e_ = access_mode::write_only;
            auto e1_ = ++e_;
            REQUIRE(e1_ == access_mode::read_only);
            REQUIRE(e_ == access_mode::read_only);

            auto e2_ = e_++;
            REQUIRE(e2_ == access_mode::read_only);
            REQUIRE(e_ == access_mode::max);
        }
    }

    {
        eee e{eee::a};
        ++e;
    }
}
