#include "kl/enum_range.hpp"
#include "kl/enum_reflector.hpp"

#include <catch2/catch.hpp>
#include <boost/optional/optional_io.hpp>

#include <string>
#include <string_view>

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
KL_REFLECT_ENUM(colour_space, rgb, xyz, ycrcb, hsv, lab, hls, luv)
} // namespace inner
} // namespace ns

enum class access_mode
{
    read_write,
    write_only,
    read_only,
    max
};
KL_REFLECT_ENUM(access_mode, read_write, write_only, read_only, max)

enum unscoped_enum_type
{
    prefix_one,
    prefix_two
};
KL_REFLECT_ENUM(unscoped_enum_type, prefix_one, (prefix_two, PREFIX_TWO))

enum class non_reflectable { one, two, three };
} // namespace

TEST_CASE("enum_reflector")
{
    SECTION("is_enum_reflectable type trait")
    {
        static_assert(!kl::is_enum_reflectable<int>::value);
        static_assert(!kl::is_enum_reflectable<non_reflectable>::value);
        static_assert(kl::is_enum_reflectable<access_mode>::value);
        static_assert(kl::is_enum_reflectable<ns::inner::colour_space>::value);
        static_assert(kl::is_enum_reflectable<unscoped_enum_type>::value);
    }

    SECTION("reflector for globally defined enum type")
    {
        using reflector = kl::enum_reflector<access_mode>;
        using namespace std::string_literals;

        REQUIRE(reflector::count() == 4);
        static_assert(reflector::count() == 4, "");

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
            REQUIRE(reflector::from_string("read_write").value() ==
                    access_mode::read_write);

            REQUIRE(!reflector::from_string("read_writ"));
            REQUIRE(reflector::from_string("max").value() == access_mode::max);
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
        using namespace std::string_literals;
        auto refl = kl::reflect<ns::inner::colour_space>();

        REQUIRE(refl.count() == 7);
        static_assert(kl::reflect<ns::inner::colour_space>().count() == 7, "");

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
            REQUIRE(refl.from_string("ycrcb").value() ==
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
        using reflector = kl::enum_reflector<unscoped_enum_type>;
        using namespace std::string_literals;

        REQUIRE(reflector::count() == 2);

        SECTION("to_string")
        {
            REQUIRE(reflector::to_string(prefix_one) == "prefix_one"s);
            REQUIRE(reflector::to_string(prefix_two) == "PREFIX_TWO"s);
        }

        SECTION("from_string")
        {
            REQUIRE(reflector::from_string("PREFIX_TWO").value() == prefix_two);

            std::string_view str{"PREFIX_TWO bla bla"};
            REQUIRE(reflector::from_string(str.substr(0, 10)).value() ==
                    prefix_two);
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

        CHECK(kl::from_string<access_mode>("read_write").value() ==
              access_mode::read_write);
    }

    SECTION("constexpr operations")
    {
        using reflector = kl::enum_reflector<unscoped_enum_type>;
        static_assert(reflector::count() == 2);
        static_assert(reflector::from_string(std::string_view{"PREFIX_TWO"}) ==
                      prefix_two);
        static_assert(reflector::to_string(prefix_two) ==
                      std::string_view{"PREFIX_TWO"});

        static_assert(reflector::constexpr_values()[0] == prefix_one);
        static_assert(reflector::constexpr_values()[1] == prefix_two);
    }
}
