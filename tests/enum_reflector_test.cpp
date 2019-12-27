#include "kl/enum_range.hpp"
#include "kl/enum_reflector.hpp"

#include <catch2/catch.hpp>
#include <boost/optional/optional_io.hpp>

#include <string>

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

KL_DEFINE_ENUM_REFLECTOR(unscoped_enum_type, (prefix_one, prefix_two))

KL_DEFINE_ENUM_REFLECTOR(access_mode, (read_write, write_only, read_only, max))

KL_DEFINE_ENUM_REFLECTOR(ns::inner, colour_space,
                         (rgb, xyz, ycrcb, hsv, lab, hls, luv))

TEST_CASE("enum_reflector")
{
    SECTION("enum_reflector for non-enums")
    {
        static_assert(!kl::is_enum_reflectable<int>::value, "???");
        static_assert(!kl::is_enum_nonreflectable<int>::value, "???");
    }

    SECTION("non-reflectable enum")
    {
        static_assert(!kl::is_enum_reflectable<non_reflectable>::value, "???");
        static_assert(kl::is_enum_nonreflectable<non_reflectable>::value, "???");
    }

    SECTION("reflector for globally defined enum type")
    {
        static_assert(kl::is_enum_reflectable<access_mode>::value, "???");
        static_assert(!kl::is_enum_nonreflectable<access_mode>::value, "???");

        using reflector = kl::enum_reflector<access_mode>;
        using namespace std::string_literals;

        REQUIRE(reflector::name() == "access_mode"s);
        REQUIRE(reflector::full_name() == "access_mode"s);
        REQUIRE(reflector::count() == 4);

        SECTION("to_string")
        {
            REQUIRE(reflector::to_string(access_mode::read_write) ==
                    "read_write"s);
            REQUIRE(reflector::to_string(access_mode::write_only) ==
                    "write_only"s);
            REQUIRE(reflector::to_string(access_mode::read_only) ==
                    "read_only"s);
            REQUIRE(reflector::to_string(access_mode::max) == "max"s);
            REQUIRE(reflector::to_string((access_mode)4) == "(unknown)"s);
        }

        SECTION("from_string")
        {
            REQUIRE(!!reflector::from_string("read_write"));
            REQUIRE(reflector::from_string("read_write").get() ==
                    access_mode::read_write);

            REQUIRE(!reflector::from_string("read_writ"));
            REQUIRE(!!reflector::from_string("max"));
            REQUIRE(reflector::from_string("max").get() == access_mode::max);
        }

        auto values = reflector::values();
        auto it = values.begin();
        REQUIRE(it != values.end());
        REQUIRE(*it++ == access_mode::read_write);
        REQUIRE(*it++ == access_mode::write_only);
        REQUIRE(*it++ == access_mode::read_only);
        REQUIRE(*it++ == access_mode::max);
        REQUIRE(it == values.end());
    }

    SECTION("reflector for enum type in namespace")
    {
        static_assert(kl::is_enum_reflectable<ns::inner::colour_space>::value, "???");
        static_assert(!kl::is_enum_nonreflectable<ns::inner::colour_space>::value, "???");

        using namespace std::string_literals;
        auto refl = kl::reflect<ns::inner::colour_space>();

        REQUIRE(refl.name() == "colour_space"s);
        REQUIRE(refl.full_name() == "ns::inner::colour_space"s);
        REQUIRE(refl.count() == 7);

        SECTION("to_string")
        {
            REQUIRE(refl.to_string(ns::inner::colour_space::rgb) == "rgb"s);
            REQUIRE(refl.to_string(ns::inner::colour_space::xyz) == "xyz"s);
            REQUIRE(refl.to_string(ns::inner::colour_space::ycrcb) == "ycrcb"s);
            REQUIRE(refl.to_string(ns::inner::colour_space::hsv) == "hsv"s);
            REQUIRE(refl.to_string(ns::inner::colour_space::lab) == "lab"s);
            REQUIRE(refl.to_string(ns::inner::colour_space::hls) == "hls"s);
            REQUIRE(refl.to_string(ns::inner::colour_space::luv) == "luv"s);
        }

        SECTION("from_string")
        {
            REQUIRE(!!refl.from_string("ycrcb"));
            REQUIRE(refl.from_string("ycrcb").get() ==
                    ns::inner::colour_space::ycrcb);
        }

        auto values = refl.values();
        auto it = values.begin();
        REQUIRE(it != values.end());
        REQUIRE(*it++ == ns::inner::colour_space::rgb);
        REQUIRE(*it++ == ns::inner::colour_space::xyz);
        REQUIRE(*it++ == ns::inner::colour_space::ycrcb);
        REQUIRE(*it++ == ns::inner::colour_space::hsv);
        REQUIRE(*it++ == ns::inner::colour_space::lab);
        REQUIRE(*it++ == ns::inner::colour_space::hls);
        REQUIRE(*it++ == ns::inner::colour_space::luv);
        REQUIRE(it == values.end());
    }

    SECTION("reflector for old enum type (unscoped)")
    {
        static_assert(kl::is_enum_reflectable<unscoped_enum_type>::value, "???");
        static_assert(!kl::is_enum_nonreflectable<unscoped_enum_type>::value, "???");

        using reflector = kl::enum_reflector<unscoped_enum_type>;
        using namespace std::string_literals;

        REQUIRE(reflector::name() == "unscoped_enum_type"s);
        REQUIRE(reflector::full_name() == "unscoped_enum_type"s);
        REQUIRE(reflector::count() == 2);

        SECTION("to_string")
        {
            REQUIRE(reflector::to_string(prefix_one) == "prefix_one"s);
            REQUIRE(reflector::to_string(prefix_two) == "prefix_two"s);
        }

        SECTION("from_string")
        {
            REQUIRE(reflector::from_string("prefix_two"));
            REQUIRE(reflector::from_string("prefix_two").get() == prefix_two);
        }

        auto values = reflector::values();
        auto it = values.begin();
        REQUIRE(it != values.end());
        REQUIRE(*it++ == prefix_one);
        REQUIRE(*it++ == prefix_two);
        REQUIRE(it == values.end());
    }

    SECTION("global to_string and from_string")
    {
        using namespace std::string_literals;

        CHECK(kl::to_string(prefix_one) == "prefix_one"s);
        CHECK(kl::to_string(ns::inner::colour_space::rgb) == "rgb"s);

        REQUIRE(!!kl::from_string<access_mode>("read_write"));
        CHECK(kl::from_string<access_mode>("read_write").get() ==
              access_mode::read_write);
    }
}
