#include "kl/json_convert.hpp"
#include "kl/ctti.hpp"

#include <catch/catch.hpp>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <tuple>

struct optional_test
{
    boost::optional<int> opt;
    int non_opt;
};
KL_DEFINE_REFLECTABLE(optional_test, (opt, non_opt))

struct inner_t
{
    int r = 1337;
    double d = 3.145926;
};
KL_DEFINE_REFLECTABLE(inner_t, (r, d))

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
KL_DEFINE_ENUM_REFLECTOR(colour_space, (rgb, xyz, ycrcb, hsv, lab, hls, luv))

// on GCC underlying_type(ordinary_enum) => unsigned
enum ordinary_enum : int { oe_one };
enum class scope_enum { one };
enum ordinary_enum_reflectable { oe_one_ref };
enum class scope_enum_reflectable { one };
KL_DEFINE_ENUM_REFLECTOR(ordinary_enum_reflectable, (oe_one_ref))
KL_DEFINE_ENUM_REFLECTOR(scope_enum_reflectable, (one))

struct enums
{
    ordinary_enum e0 = ordinary_enum::oe_one;
    scope_enum e1 = scope_enum::one;
    ordinary_enum_reflectable e2 = ordinary_enum_reflectable::oe_one_ref;
    scope_enum_reflectable e3 = scope_enum_reflectable::one;
};
KL_DEFINE_REFLECTABLE(enums, (e0, e1, e2, e3))

struct test_t
{
    std::string hello = "world";
    bool t = true;
    bool f = false;
    boost::optional<int> n;
    int i = 123;
    double pi = 3.1416;
    std::vector<int> a = {1,2,3,4};
    std::vector<std::vector<int>> ad = {std::vector<int>{1, 2},
                                        std::vector<int>{3, 4, 5}};
    colour_space space = colour_space::lab;
    std::tuple<int, double, std::string> tup = std::make_tuple(1, 3.14f, "QWE");

    std::map<std::string, colour_space> map = {{"1", colour_space::hls},
                                               {"2", colour_space::rgb}};

    inner_t inner;
};

KL_DEFINE_REFLECTABLE(test_t,
                      (hello, t, f, n, i, pi, a, ad, space, tup, map, inner))

TEST_CASE("json_convert")
{
    std::string err;

    SECTION("serialize inner_t")
    {
        auto j = kl::to_json<inner_t>(inner_t{});
        REQUIRE(j.is_object());
        REQUIRE(j.has_shape(
            {{"r", json11::Json::NUMBER}, {"d", json11::Json::NUMBER}}, err));
        REQUIRE(j["r"] == 1337);
        REQUIRE(j["d"] == 3.145926);
    }

    SECTION("deserialize inner_t - empty json")
    {
        REQUIRE_THROWS_AS(kl::from_json<inner_t>({}),
                          kl::json_deserialize_exception);
    }

    SECTION("deserialize inner_t - missing one field")
    {
        const char* in = R"({"d": 1.0})";
        auto j = json11::Json::parse(in, err);
        REQUIRE_THROWS_AS(kl::from_json<inner_t>(j),
                          kl::json_deserialize_exception);
    }

    SECTION("deserialize inner_t - one additional field")
    {
        const char* in = R"({"d": 1.0, "r": 2, "zzz": null})";
        auto j = json11::Json::parse(in, err);
        auto obj = kl::from_json<inner_t>(j);
        REQUIRE(obj.r == 2);
        REQUIRE(obj.d == 1.0);
    }

    SECTION("deserialize inner_t")
    {
        const char* in = R"({"d": 1.0, "r": 2})";
        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        auto obj = kl::from_json<inner_t>(j);
        REQUIRE(obj.r == 2);
        REQUIRE(obj.d == 1.0);
    }

    SECTION("serialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, false);
        auto j = kl::to_json(t);
        REQUIRE(j.is_array());
        REQUIRE(j.array_items().size() == 4);
        REQUIRE(j.array_items()[0] == 13);
        REQUIRE(j.array_items()[1] == 3.14);
        REQUIRE(j.array_items()[2] == "lab");
        REQUIRE(j.array_items()[3] == false);
    }

    SECTION("deserialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, false);
        auto j = kl::to_json(t);

        auto obj = kl::from_json<decltype(t)>(j);
        REQUIRE(std::get<0>(obj) == 13);
        REQUIRE(std::get<1>(obj) == 3.14);
        REQUIRE(std::get<2>(obj) == colour_space::lab);
        REQUIRE(std::get<3>(obj) == false);

        j = json11::Json::parse(R"([7, 13, "rgb", true])", err);
        REQUIRE(err.empty());
        obj = kl::from_json<decltype(t)>(j);
        REQUIRE(std::get<0>(obj) == 7);
        REQUIRE(std::get<1>(obj) == 13.0);
        REQUIRE(std::get<2>(obj) == colour_space::rgb);
        REQUIRE(std::get<3>(obj) == true);

        j = json11::Json::parse(R"([7, 13, true])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<decltype(t)>(j),
                          kl::json_deserialize_exception);

        j = json11::Json::parse(R"([7, 13, "rgb", 1, true])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<decltype(t)>(j),
                          kl::json_deserialize_exception);
    }

    SECTION("serialize different types and 'modes' for enums")
    {
        auto j = kl::to_json(enums{});
        REQUIRE(j["e0"] == 0);
        REQUIRE(j["e1"] == 0);
        REQUIRE(j["e2"] == "oe_one_ref");
        REQUIRE(j["e3"] == "one");
    }

    SECTION("deserialize different types and 'modes' for enums")
    {
        const char* in =
            R"({"e0": 0, "e1": 0, "e2": "oe_one_ref", "e3": "one"})";
        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());

        auto obj = kl::from_json<enums>(j);
        REQUIRE(obj.e0 == ordinary_enum::oe_one);
        REQUIRE(obj.e1 == scope_enum::one);
        REQUIRE(obj.e2 == ordinary_enum_reflectable::oe_one_ref);
        REQUIRE(obj.e3 == scope_enum_reflectable::one);
    }

    SECTION("deserialize different types and 'modes' for enums - fail")
    {
        const char* in =
            R"({"e0": 0, "e1": 0, "e2": "oe_one_ref", "e3": 0})";
        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());

        REQUIRE_THROWS_AS(kl::from_json<enums>(j),
                          kl::json_deserialize_exception);

        in = R"({"e0": 0, "e1": 0, "e2": "oe_one_ref2", "e3": 0})";
        j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<enums>(j),
                          kl::json_deserialize_exception);

        in =
            R"({"e0": 0, "e1": true, "e2": "oe_one_ref", "e3": "one"})";
        j = json11::Json::parse(in, err);
        REQUIRE(err.empty());

        REQUIRE_THROWS_AS(kl::from_json<enums>(j),
                          kl::json_deserialize_exception);
    }

#if defined(KL_JSON_CONVERT_DONT_SKIP_NULL_VALUES)
    SECTION("serialize optional fields with null")
    {
        optional_test t;
        t.non_opt = 23;

        REQUIRE(kl::to_json(t).is_object());
        REQUIRE(kl::to_json(t).object_items().size() == 2);
        REQUIRE(kl::to_json(t)["non_opt"] == 23);
        REQUIRE(kl::to_json(t)["opt"] == nullptr);

        t.opt = 78;
        REQUIRE(kl::to_json(t)["non_opt"] == 23);
        REQUIRE(kl::to_json(t)["opt"] == 78);
    }
#else
    SECTION("skip serializing optional fields")
    {

        optional_test t;
        t.non_opt = 23;

        REQUIRE(kl::to_json(t).is_object());
        REQUIRE(kl::to_json(t).object_items().size() == 1);
        REQUIRE(kl::to_json(t)["non_opt"] == 23);

        t.opt = 78;
        REQUIRE(kl::to_json(t)["non_opt"] == 23);
        REQUIRE(kl::to_json(t)["opt"] == 78);
    }
#endif

    SECTION("deserialize fields with null")
    {
        auto in = R"({"opt": null, "non_opt": 3})";
        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        auto obj = kl::from_json<optional_test>(j);
        REQUIRE(!obj.opt);
        REQUIRE(obj.non_opt == 3);

        in = R"({"opt": 4, "non_opt": 13})";
        j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        obj = kl::from_json<optional_test>(j);
        REQUIRE(obj.opt);
        REQUIRE(*obj.opt == 4);
        REQUIRE(obj.non_opt == 13);
    }

    SECTION("deserialize with optional fields missing")
    {
        auto in = R"({"non_opt": 32})";
        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        auto obj = kl::from_json<optional_test>(j);
        REQUIRE(obj.non_opt == 32);
        REQUIRE(!obj.opt);
    }

    SECTION("deserialize with optional fields invalid")
    {
        auto in = R"({"non_opt": 32, "opt": "QWE"})";
        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());

        try
        {
            kl::from_json<optional_test>(j);
        }
        catch (std::exception& ex)
        {
            ex.what();
        }

        REQUIRE_THROWS_AS(kl::from_json<optional_test>(j),
                          kl::json_deserialize_exception);
    }

    SECTION("serialize complex structure with std/boost containers")
    {
        test_t t;
        auto j = kl::to_json(t);
        REQUIRE(j.is_object());
        REQUIRE(j["hello"] == "world");
        REQUIRE(j["t"] == true);
        REQUIRE(j["f"] == false);
        REQUIRE(j["n"] == nullptr);
        REQUIRE(j["i"] == 123);
        REQUIRE(j["pi"] == 3.1416);
        REQUIRE(j["space"] == "lab");

        REQUIRE(j["a"].is_array());
        auto a = j["a"].array_items();
        REQUIRE(a.size() == 4);
        REQUIRE(a[0] == 1);
        REQUIRE(a[3] == 4);

        REQUIRE(j["ad"].is_array());
        auto ad = j["ad"].array_items();
        REQUIRE(ad.size() == 2);
        REQUIRE(ad[0].is_array());
        REQUIRE(ad[1].is_array());
        REQUIRE(ad[0].array_items().size() == 2);
        REQUIRE(ad[0].array_items()[0] == 1);
        REQUIRE(ad[0].array_items()[1] == 2);
        REQUIRE(ad[1].array_items().size() == 3);
        REQUIRE(ad[1].array_items()[0] == 3);
        REQUIRE(ad[1].array_items()[1] == 4);
        REQUIRE(ad[1].array_items()[2] == 5);

        REQUIRE(j["tup"].is_array());
        REQUIRE(j["tup"].array_items().size() == 3);
        REQUIRE(j["tup"].array_items()[0] == 1);
        REQUIRE(j["tup"].array_items()[1] == 3.14f);
        REQUIRE(j["tup"].array_items()[2] == "QWE");

        REQUIRE(j["map"].is_object());
        REQUIRE(j["map"].object_items().find("1")->second == "hls");
        REQUIRE(j["map"].object_items().find("2")->second == "rgb");

        REQUIRE(j["inner"].is_object());
        auto inner = j["inner"].object_items();
        REQUIRE(inner["r"] == inner_t{}.r);
        REQUIRE(inner["d"] == inner_t{}.d);
    }

    SECTION("deserialize complex structure")
    {
        const char* in = R"(
{
  "a": [
    10,
    20,
    30,
    40
  ],
  "ad": [
    [
      20
    ],
    [
      30,
      40,
      50
    ]
  ],
  "f": true,
  "hello": "new world",
  "i": 456,
  "inner": {
    "d": 2.71,
    "r": 667
  },
  "map": {
    "10": "xyz",
    "20": "lab"
  },
  "n": 3,
  "pi": 3.1415999999999999,
  "space": "rgb",
  "t": false,
  "tup": [
    10,
    31.4,
    "ASD"
  ]
}
)";

        auto j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        auto obj = kl::from_json<test_t>(j);

        REQUIRE(obj.a == (std::vector<int>{10, 20, 30, 40}));
        REQUIRE(obj.ad ==
                (std::vector<std::vector<int>>{std::vector<int>{20},
                                               std::vector<int>{30, 40, 50}}));
        REQUIRE(obj.f == true);
        REQUIRE(obj.hello == "new world");
        REQUIRE(obj.i == 456);
        REQUIRE(obj.inner.d == 2.71);
        REQUIRE(obj.inner.r == 667);
        REQUIRE(obj.map ==
                (std::map<std::string, colour_space>{
                    {"10", colour_space::xyz}, {"20", colour_space::lab}}));
        REQUIRE(obj.n);
        REQUIRE(*obj.n == 3);
        REQUIRE(obj.pi == 3.1416);
        REQUIRE(obj.space == colour_space::rgb);
        REQUIRE(obj.t == false);
        using namespace std::string_literals;
        REQUIRE(obj.tup == std::make_tuple(10, 31.4, "ASD"s));
    }
}
