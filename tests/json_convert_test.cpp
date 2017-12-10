#include "kl/json_convert.hpp"
#include "kl/ctti.hpp"

#include <catch/catch.hpp>
#include <boost/optional/optional_io.hpp>

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

struct unsigned_test
{
    unsigned char u8{128};
    unsigned short u16{32768};
    unsigned int u32{std::numeric_limits<unsigned int>::max()};
    // unsigned long long u64{0}; // can't represent losslessly u64 in double
};
KL_DEFINE_REFLECTABLE(unsigned_test, (u8, u16, u32))

TEST_CASE("json_convert")
{
    std::string err;

    SECTION("serialize inner_t")
    {
        auto j = kl::to_json(inner_t{});
        REQUIRE(j.is_object());
        REQUIRE(j.has_shape(
            {{"r", json11::Json::NUMBER}, {"d", json11::Json::NUMBER}}, err));
        REQUIRE(j["r"] == 1337);
        REQUIRE(j["d"] == 3.145926);
    }

    SECTION("deserialize inner_t - empty json")
    {
        REQUIRE_THROWS_AS(kl::from_json<inner_t>({}),
                          kl::json_deserialize_error);
    }

    SECTION("deserialize inner_t - missing one field")
    {
        const char* in = R"({"d": 1.0})";
        auto j = json11::Json::parse(in, err);
        REQUIRE_THROWS_AS(kl::from_json<inner_t>(j),
                          kl::json_deserialize_error);
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

    SECTION("deserialize simple - wrong types")
    {
        json11::Json null;
        REQUIRE_THROWS_AS(kl::from_json<int>(null),
                          kl::json_deserialize_error);
        REQUIRE_THROWS_AS(kl::from_json<bool>(null),
                          kl::json_deserialize_error);
        REQUIRE_THROWS_AS(kl::from_json<float>(null),
                          kl::json_deserialize_error);
        REQUIRE_THROWS_AS(kl::from_json<std::string>(null),
                          kl::json_deserialize_error);
        REQUIRE_THROWS_AS(kl::from_json<std::tuple<int>>(null),
                          kl::json_deserialize_error);
        REQUIRE_THROWS_AS(kl::from_json<std::vector<int>>(null),
                          kl::json_deserialize_error);
        REQUIRE_THROWS_AS((kl::from_json<std::map<std::string, int>>(null)),
                          kl::json_deserialize_error);

        json11::Json arr{json11::Json::array{3, true}};
        REQUIRE_THROWS_AS(kl::from_json<std::vector<int>>(arr),
                          kl::json_deserialize_error);

        json11::Json obj{json11::Json::object{{"key0", 3}, {"key2", true}}};
        REQUIRE_THROWS_AS((kl::from_json<std::map<std::string, int>>(obj)),
                          kl::json_deserialize_error);
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
                          kl::json_deserialize_error);

        j = json11::Json::parse(R"([7, 13, "rgb", 1, true])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<decltype(t)>(j),
                          kl::json_deserialize_error);
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
                          kl::json_deserialize_error);

        in = R"({"e0": 0, "e1": 0, "e2": "oe_one_ref2", "e3": 0})";
        j = json11::Json::parse(in, err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<enums>(j),
                          kl::json_deserialize_error);

        in =
            R"({"e0": 0, "e1": true, "e2": "oe_one_ref", "e3": "one"})";
        j = json11::Json::parse(in, err);
        REQUIRE(err.empty());

        REQUIRE_THROWS_AS(kl::from_json<enums>(j),
                          kl::json_deserialize_error);
    }

#if defined(KL_JSON_DONT_SKIP_NULL_VALUES)
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
            REQUIRE(!strcmp(ex.what(),
                            "type must be an integral but is STRING\n"
                            "error when deserializing field opt"));
        }

        REQUIRE_THROWS_AS(kl::from_json<optional_test>(j),
                          kl::json_deserialize_error);
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

    SECTION("test unsigned types")
    {
        unsigned_test t;
        auto j = kl::to_json(t);

        REQUIRE(j["u8"].number_value() == t.u8);
        REQUIRE(j["u16"].number_value() == t.u16);
        REQUIRE(j["u32"].number_value() == t.u32);

        auto obj = kl::from_json<unsigned_test>(j);
        REQUIRE(obj.u8 == t.u8);
        REQUIRE(obj.u16 == t.u16);
        REQUIRE(obj.u32 == t.u32);
    }

    SECTION("deserialize to struct from an array")
    {
        auto j = json11::Json::parse(R"([3,4.0])", err);
        REQUIRE(err.empty());
        auto obj = kl::from_json<inner_t>(j);
        REQUIRE(obj.r == 3);
        REQUIRE(obj.d == 4.0);
    }

    SECTION("deserialize to struct from an array - num elements differs")
    {
        auto j = json11::Json::parse(R"([3,4.0,"QWE"])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<inner_t>(j),
                          kl::json_deserialize_error);

        j = json11::Json::parse(R"([3])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<inner_t>(j),
                          kl::json_deserialize_error);
    }

    SECTION("deserialize to struct from an array - type mismatch")
    {
        auto j = json11::Json::parse(R"([3,"QWE"])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<inner_t>(j),
                          kl::json_deserialize_error);

        j = json11::Json::parse(R"([false,4])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<inner_t>(j),
                          kl::json_deserialize_error);
    }

    SECTION("optional<string>")
    {
        auto j = json11::Json::parse(R"(null)", err);
        REQUIRE(err.empty());
        auto a = kl::from_json<boost::optional<std::string>>(j);
        REQUIRE(!a);

        j = json11::Json::parse(R"("asd")", err);
        REQUIRE(err.empty());
        auto b = kl::from_json<boost::optional<std::string>>(j);
        REQUIRE(b);
        REQUIRE(b.get() == "asd");
    }


    SECTION("tuple with tail optionals")
    {
        using tuple_t = std::tuple<int, bool, boost::optional<std::string>>;

        auto j = json11::Json::parse(R"([4])", err);
        REQUIRE(err.empty());
        REQUIRE_THROWS_AS(kl::from_json<tuple_t>(j),
                          kl::json_deserialize_error);

        j = json11::Json::parse(R"([4,true])", err);
        REQUIRE(err.empty());

        auto t = kl::from_json<tuple_t>(j);
        REQUIRE(std::get<0>(t) == 4);
        REQUIRE(std::get<1>(t));
        REQUIRE(!std::get<2>(t).is_initialized());
    }

    SECTION("unsigned: check 32")
    {
        auto j = kl::to_json(unsigned(32));
        REQUIRE(j.number_value() == 32.0);
        REQUIRE(j.int_value() == 32);
        auto str = j.pretty_print();
        REQUIRE(str == "32");
    }

    SECTION("unsigned: check value greater than 0x7FFFFFFU")
    {
        auto j = kl::to_json(2147483648U);
        REQUIRE(j.number_value() == 2147483648.0);
        REQUIRE(j.int_value() == -2147483648LL);
        auto str = j.pretty_print();
        REQUIRE(str == "2147483648");
    }
}

#include <chrono>

class our_type
{
public:
    our_type() = default;

    // These two functions must be present in order to json_convert to work with
    // them without KL_DEFINE_REFLECTABLE macro
    static our_type from_json(const json11::Json& json)
    {
        return our_type(kl::from_json<int>(json["i"]),
                        kl::from_json<double>(json["d"]),
                        kl::from_json<std::string>(json["str"]));
    }

    json11::Json to_json() const
    {
        return json11::Json::object{
            {"i", i},
            {"d", d},
            {"str", str}
        };
    }

private:
    our_type(int i, double d, std::string str)
        : i(i), d(d), str(std::move(str))
    {
    }

private:
    int i{10};
    double d{3.14};
    std::string str = "zxc";
};

struct chrono_test
{
    int t;
    std::chrono::seconds sec;
    std::vector<std::chrono::seconds> secs;
    our_type o;
};
KL_DEFINE_REFLECTABLE(chrono_test, (t, sec, secs, o))

// Specialize to_json/from_json for std::chrono::seconds
// Don't overload since from_json only differs by return type
namespace kl {
template <>
json11::Json to_json(const std::chrono::seconds& t)
{
    return {static_cast<double>(t.count())};
}

template <>
std::chrono::seconds from_json(const json11::Json& json)
{
    return std::chrono::seconds{from_json<unsigned>(json)};
}
} // namespace kl

TEST_CASE("json_convert - extended")
{
    using namespace std::chrono;
    chrono_test t{2, seconds{10}, {seconds{10}, seconds{10}}, our_type{}};
    auto j = kl::to_json(t);
    auto obj = kl::from_json<chrono_test>(j);
}
