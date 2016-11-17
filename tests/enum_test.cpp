#include "kl/enum_traits.hpp"
#include "kl/enum_range.hpp"
#include "kl/enum_reflector.hpp"

#include <catch/catch.hpp>
#include <string>

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

KL_DEFINE_ENUM_REFLECTOR(unscoped_enum_type, (prefix_one, prefix_two))

KL_DEFINE_ENUM_REFLECTOR(access_mode, (read_write, write_only, read_only, max))

KL_DEFINE_ENUM_REFLECTOR(ns::inner, colour_space,
                         (rgb, xyz, ycrcb, hsv, lab, hls, luv))

TEST_CASE("enum_traits")
{
    static_assert(kl::enum_traits<access_mode>::min_value() == 0, "");
    static_assert(kl::enum_traits<access_mode>::max_value() == 2 + 1, "");
    static_assert(kl::enum_traits<access_mode>::min() == access_mode::read_write, "");
    static_assert(kl::enum_traits<access_mode>::max() == access_mode::max, "");
    static_assert(kl::enum_traits<access_mode>::count() == 3, "");

    static_assert(
        kl::enum_traits<access_mode>::in_range(access_mode::read_only), "");
    static_assert(!kl::enum_traits<access_mode>::in_range(access_mode::max), "");
    static_assert(!kl::enum_traits<access_mode>::in_range(34), "");
    static_assert(kl::enum_traits<access_mode>::in_range(2), "");

    using namespace ns::inner;
    static_assert(kl::enum_traits<colour_space>::min_value() == 2, "");
    static_assert(kl::enum_traits<colour_space>::max_value() == 8 + 1, "");
    static_assert(kl::enum_traits<colour_space>::min() == colour_space::rgb, "");
    static_assert(kl::enum_traits<colour_space>::max() ==
                      static_cast<colour_space>(+colour_space::luv + 1),
                  "");
    static_assert(kl::enum_traits<colour_space>::count() == 7, "");
    static_assert(
        kl::enum_traits<colour_space>::in_range(colour_space::luv), "");
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

#if !defined(_MSC_VER)
        auto e2 = e++;
        REQUIRE(e2 == colour_space::hls);
        REQUIRE(e == colour_space::luv);
#endif

        {
            using namespace kl::enums::operators;
            REQUIRE(+access_mode::max == 3);

            auto e_ = access_mode::write_only;
            auto e1_ = ++e_;
            REQUIRE(e1_ == access_mode::read_only);
            REQUIRE(e_ == access_mode::read_only);

#if !defined(_MSC_VER)
            auto e2_ = e_++;
            REQUIRE(e2_ == access_mode::read_only);
            REQUIRE(e_ == access_mode::max);
#endif
        }
    }

    {
        eee e{eee::a};
        ++e;
    }
}

TEST_CASE("enum_reflector")
{
    SECTION("non-reflectable enum")
    {
        static_assert(!kl::is_enum_reflectable<non_reflectable>::value, "???");
    }

    SECTION("reflector for globally defined enum type")
    {
        static_assert(kl::is_enum_reflectable<access_mode>::value, "???");

        using reflector = typename kl::enum_reflector<access_mode>;
        using namespace std::string_literals;

        REQUIRE(reflector::name() == "access_mode"s);
        REQUIRE(reflector::full_name() == "access_mode"s);
        REQUIRE(reflector::count() == 4);

        SECTION("to_string")
        {
            REQUIRE(reflector::to_string(access_mode::read_write) == "read_write"s);
            REQUIRE(reflector::to_string(access_mode::write_only) == "write_only"s);
            REQUIRE(reflector::to_string(access_mode::read_only) == "read_only"s);
            REQUIRE(reflector::to_string(access_mode::max) == "max"s);
            REQUIRE(reflector::to_string((access_mode)4) == "(unknown)"s);
        }

        SECTION("from_string")
        {
            REQUIRE(reflector::from_string("read_write"));
            REQUIRE(reflector::from_string("read_write").get() ==
                    access_mode::read_write);

            REQUIRE(!reflector::from_string("read_writ"));
            REQUIRE(reflector::from_string("max"));
            REQUIRE(reflector::from_string("max").get() == access_mode::max);
        }
    }

    SECTION("reflector for enum type in namespace")
    {
        static_assert(kl::is_enum_reflectable<ns::inner::colour_space>::value, "???");

        using reflector = typename kl::enum_reflector<ns::inner::colour_space>;
        using namespace std::string_literals;

        REQUIRE(reflector::name() == "colour_space"s);
        REQUIRE(reflector::full_name() == "ns::inner::colour_space"s);
        REQUIRE(reflector::count() == 7);

        SECTION("to_string")
        {
            REQUIRE(reflector::to_string(ns::inner::colour_space::rgb) == "rgb"s);
            REQUIRE(reflector::to_string(ns::inner::colour_space::xyz) == "xyz"s);
            REQUIRE(reflector::to_string(ns::inner::colour_space::ycrcb) == "ycrcb"s);
            REQUIRE(reflector::to_string(ns::inner::colour_space::hsv) == "hsv"s);
            REQUIRE(reflector::to_string(ns::inner::colour_space::lab) == "lab"s);
            REQUIRE(reflector::to_string(ns::inner::colour_space::hls) == "hls"s);
            REQUIRE(reflector::to_string(ns::inner::colour_space::luv) == "luv"s);
        }

        SECTION("from_string")
        {
            REQUIRE(reflector::from_string("ycrcb"));
            REQUIRE(reflector::from_string("ycrcb").get() ==
                    ns::inner::colour_space::ycrcb);
        }
    }

    SECTION("reflector for old enum type (unscoped)")
    {
        static_assert(kl::is_enum_reflectable<unscoped_enum_type>::value, "???");

        using reflector = typename kl::enum_reflector<unscoped_enum_type>;
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
    }
}

TEST_CASE("enum_range")
{
    SECTION("access_mode with ::max")
    {
        using reflector = typename kl::enum_reflector<access_mode>;

        std::vector<access_mode> vec;

        for (const auto v : kl::enum_range<access_mode>())
        {
            vec.push_back(v);
        }

        REQUIRE(vec.size() == 3);
        REQUIRE(vec.size() == reflector::count() - 1); // max
        REQUIRE(vec[0] == access_mode::read_write);
        REQUIRE(vec[1] == access_mode::write_only);
        REQUIRE(vec[2] == access_mode::read_only);
    }

    SECTION("colour_space without ::max")
    {
        using ns::inner::colour_space;
        using reflector = typename kl::enum_reflector<colour_space>;

        std::vector<colour_space> vec;

        for (const auto v : kl::enum_range<colour_space>())
        {
            vec.push_back(v);
        }

        REQUIRE(vec.size() == 7);
        REQUIRE(vec.size() == reflector::count());
        REQUIRE(vec[0] == colour_space::rgb);
        REQUIRE(vec[1] == colour_space::xyz);
        REQUIRE(vec[2] == colour_space::ycrcb);
        REQUIRE(vec[3] == colour_space::hsv);
        REQUIRE(vec[4] == colour_space::lab);
        REQUIRE(vec[5] == colour_space::hls);
        REQUIRE(vec[6] == colour_space::luv);
    }
}
