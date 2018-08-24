#include "kl/json_convert.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_flags.hpp"

#include <catch/catch.hpp>
#include <boost/optional/optional_io.hpp>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <tuple>
#include <deque>
#include <list>

struct optional_test
{
    int non_opt;
    boost::optional<int> opt;
};
KL_DEFINE_REFLECTABLE(optional_test, (non_opt, opt))

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
    std::vector<int> a = {1, 2, 3, 4};
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
    using namespace kl;

    SECTION("serialize inner_t")
    {
        auto j = json::serialize(inner_t{});
        REQUIRE(j.is_object());

        std::string err;
        REQUIRE(j.has_shape(
            {{"r", json11::Json::NUMBER}, {"d", json11::Json::NUMBER}}, err));
        REQUIRE(j["r"] == 1337);
        REQUIRE(j["d"] == 3.145926);
    }

    SECTION("deserialize inner_t - empty json")
    {
        REQUIRE_THROWS_AS(json::deserialize<inner_t>({}),
                          json::deserialize_error&);
    }

    SECTION("deserialize inner_t - missing one field")
    {
        auto j = R"({"d": 1.0})"_json;
        REQUIRE_THROWS_AS(json::deserialize<inner_t>(j),
                          json::deserialize_error&);
    }

    SECTION("deserialize inner_t - one additional field")
    {
        auto j = R"({"d": 1.0, "r": 2, "zzz": null})"_json;
        auto obj = json::deserialize<inner_t>(j);
        REQUIRE(obj.r == 2);
        REQUIRE(obj.d == 1.0);
    }

    SECTION("deserialize inner_t")
    {
        auto j = R"({"d": 1.0, "r": 2})"_json;
        auto obj = json::deserialize<inner_t>(j);
        REQUIRE(obj.r == 2);
        REQUIRE(obj.d == 1.0);
    }

    SECTION("serialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, false);
        auto j = json::serialize(t);
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
        REQUIRE_THROWS_AS(json::deserialize<int>(null),
                          json::deserialize_error&);
        REQUIRE_THROWS_AS(json::deserialize<bool>(null),
                          json::deserialize_error&);
        REQUIRE_THROWS_AS(json::deserialize<float>(null),
                          json::deserialize_error&);
        REQUIRE_THROWS_AS(json::deserialize<std::string>(null),
                          json::deserialize_error&);
        REQUIRE_THROWS_AS(json::deserialize<std::tuple<int>>(null),
                          json::deserialize_error&);
        REQUIRE_THROWS_AS(json::deserialize<std::vector<int>>(null),
                          json::deserialize_error&);
        REQUIRE_THROWS_AS((json::deserialize<std::map<std::string, int>>(null)),
                          json::deserialize_error&);

        json11::Json arr{json11::Json::array{3, true}};
        REQUIRE_THROWS_AS(json::deserialize<std::vector<int>>(arr),
                          json::deserialize_error&);

        json11::Json obj{json11::Json::object{{"key0", 3}, {"key2", true}}};
        REQUIRE_THROWS_AS((json::deserialize<std::map<std::string, int>>(obj)),
                          json::deserialize_error&);
    }

    SECTION("deserialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, false);
        auto j = kl::json::serialize(t);

        auto obj = json::deserialize<decltype(t)>(j);
        REQUIRE(std::get<0>(obj) == 13);
        REQUIRE(std::get<1>(obj) == 3.14);
        REQUIRE(std::get<2>(obj) == colour_space::lab);
        REQUIRE(std::get<3>(obj) == false);

        j = R"([7, 13, "rgb", true])"_json;
        obj = json::deserialize<decltype(t)>(j);
        REQUIRE(std::get<0>(obj) == 7);
        REQUIRE(std::get<1>(obj) == 13.0);
        REQUIRE(std::get<2>(obj) == colour_space::rgb);
        REQUIRE(std::get<3>(obj) == true);

        j = R"([7, 13, true])"_json;
        REQUIRE_THROWS_AS(json::deserialize<decltype(t)>(j),
                          json::deserialize_error&);

        j = R"([7, 13, "rgb", 1, true])"_json;
        REQUIRE_THROWS_AS(json::deserialize<decltype(t)>(j),
                          json::deserialize_error&);
    }

    SECTION("serialize different types and 'modes' for enums")
    {
        auto j = json::serialize(enums{});
        REQUIRE(j["e0"] == 0);
        REQUIRE(j["e1"] == 0);
        REQUIRE(j["e2"] == "oe_one_ref");
        REQUIRE(j["e3"] == "one");
    }

    SECTION("deserialize different types and 'modes' for enums")
    {
        auto j = R"({"e0": 0, "e1": 0, "e2": "oe_one_ref", "e3": "one"})"_json;

        auto obj = json::deserialize<enums>(j);
        REQUIRE(obj.e0 == ordinary_enum::oe_one);
        REQUIRE(obj.e1 == scope_enum::one);
        REQUIRE(obj.e2 == ordinary_enum_reflectable::oe_one_ref);
        REQUIRE(obj.e3 == scope_enum_reflectable::one);
    }

    SECTION("deserialize different types and 'modes' for enums - fail")
    {
        auto j = R"({"e0": 0, "e1": 0, "e2": "oe_one_ref", "e3": 0})"_json;
        REQUIRE_THROWS_AS(json::deserialize<enums>(j),
                          json::deserialize_error&);

        j = R"({"e0": 0, "e1": 0, "e2": "oe_one_ref2", "e3": 0})"_json;
        REQUIRE_THROWS_AS(json::deserialize<enums>(j),
                          json::deserialize_error&);

        j = R"({"e0": 0, "e1": true, "e2": "oe_one_ref", "e3": "one"})"_json;
        REQUIRE_THROWS_AS(json::deserialize<enums>(j),
                          json::deserialize_error&);
    }

    SECTION("skip serializing optional fields")
    {
        optional_test t;
        t.non_opt = 23;

        REQUIRE(json::serialize(t).is_object());
        REQUIRE(json::serialize(t).object_items().size() == 1);
        REQUIRE(json::serialize(t)["non_opt"] == 23);

        t.opt = 78;
        REQUIRE(json::serialize(t)["non_opt"] == 23);
        REQUIRE(json::serialize(t)["opt"] == 78);
    }

    SECTION("deserialize fields with null")
    {
        auto j = R"({"opt": null, "non_opt": 3})"_json;
        auto obj = json::deserialize<optional_test>(j);
        REQUIRE(!obj.opt);
        REQUIRE(obj.non_opt == 3);

        j = R"({"opt": 4, "non_opt": 13})"_json;
        obj = json::deserialize<optional_test>(j);
        REQUIRE(obj.opt);
        REQUIRE(*obj.opt == 4);
        REQUIRE(obj.non_opt == 13);
    }

    SECTION("deserialize with optional fields missing")
    {
        auto j = R"({"non_opt": 32})"_json;
        auto obj = json::deserialize<optional_test>(j);
        REQUIRE(obj.non_opt == 32);
        REQUIRE(!obj.opt);
    }

    SECTION("deserialize with optional fields invalid")
    {
        auto j = R"({"non_opt": 32, "opt": "QWE"})"_json;

        try
        {
            json::deserialize<optional_test>(j);
        }
        catch (std::exception& ex)
        {
            REQUIRE(!strcmp(ex.what(),
                            "type must be an integral but is STRING\n"
                            "error when deserializing field opt\n"
                            "error when deserializing type optional_test"));
        }

        REQUIRE_THROWS_AS(json::deserialize<optional_test>(j),
                          json::deserialize_error&);
    }

    SECTION("serialize complex structure with std/boost containers")
    {
        test_t t;
        auto j = json::serialize(t);
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
        auto j = R"(
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
)"_json;

        auto obj = json::deserialize<test_t>(j);

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
        auto j = json::serialize(t);

        REQUIRE(j["u8"].number_value() == t.u8);
        REQUIRE(j["u16"].number_value() == t.u16);
        REQUIRE(j["u32"].number_value() == t.u32);

        auto obj = json::deserialize<unsigned_test>(j);
        REQUIRE(obj.u8 == t.u8);
        REQUIRE(obj.u16 == t.u16);
        REQUIRE(obj.u32 == t.u32);
    }

    SECTION("deserialize to struct from an array")
    {
        auto j = R"([3,4.0])"_json;
        auto obj = json::deserialize<inner_t>(j);
        REQUIRE(obj.r == 3);
        REQUIRE(obj.d == 4.0);
    }

    SECTION("deserialize to struct from an array - num elements differs")
    {
        auto j = R"([3,4.0,"QWE"])"_json;
        REQUIRE_THROWS_AS(json::deserialize<inner_t>(j),
                          json::deserialize_error&);

        j = R"([3])"_json;
        REQUIRE_THROWS_AS(json::deserialize<inner_t>(j),
                          json::deserialize_error&);
    }

    SECTION("deserialize to struct from an array - tail optional fields")
    {
        auto j = R"([234])"_json;
        auto obj = json::deserialize<optional_test>(j);
        REQUIRE(obj.non_opt == 234);
        REQUIRE(!obj.opt);
    }

    SECTION("deserialize to struct from an array - type mismatch")
    {
        auto j = R"([3,"QWE"])"_json;
        REQUIRE_THROWS_AS(json::deserialize<inner_t>(j),
                          json::deserialize_error&);

        j = R"([false,4])"_json;
        REQUIRE_THROWS_AS(json::deserialize<inner_t>(j),
                          json::deserialize_error&);
    }

    SECTION("optional<string>")
    {
        auto j = R"(null)"_json;
        auto a = json::deserialize<boost::optional<std::string>>(j);
        REQUIRE(!a);

        j = R"("asd")"_json;
        auto b = json::deserialize<boost::optional<std::string>>(j);
        REQUIRE(b);
        REQUIRE(b.get() == "asd");
    }

    SECTION("tuple with tail optionals")
    {
        using tuple_t = std::tuple<int, bool, boost::optional<std::string>>;

        auto j = R"([4])"_json;
        REQUIRE_THROWS_AS(json::deserialize<tuple_t>(j),
                          json::deserialize_error&);

        j = R"([4,true])"_json;
        auto t = json::deserialize<tuple_t>(j);
        REQUIRE(std::get<0>(t) == 4);
        REQUIRE(std::get<1>(t));
        REQUIRE(!std::get<2>(t).is_initialized());
    }

    SECTION("unsigned: check 32")
    {
        auto j = json::serialize(unsigned(32));
        REQUIRE(j.number_value() == 32.0);
        REQUIRE(j.int_value() == 32);
        auto str = j.pretty_print();
        REQUIRE(str == "32");
    }

    SECTION("unsigned: check value greater than 0x7FFFFFFU")
    {
        auto j = json::serialize(2147483648U);
        REQUIRE(j.number_value() == 2147483648.0);
        REQUIRE(j.int_value() == -2147483648LL);
        auto str = j.pretty_print();
        REQUIRE(str == "2147483648");
    }

    SECTION("to std containers")
    {
        auto j1 = json::serialize(std::vector<inner_t>{inner_t{}});
        REQUIRE(j1.type() == json11::Json::ARRAY);
        REQUIRE(j1.array_items().size() == 1);

        auto j2 = json::serialize(std::list<inner_t>{inner_t{}});
        REQUIRE(j2.type() == json11::Json::ARRAY);
        REQUIRE(j2.array_items().size() == 1);

        auto j3 = json::serialize(std::deque<inner_t>{inner_t{}});
        REQUIRE(j3.type() == json11::Json::ARRAY);
        REQUIRE(j3.array_items().size() == 1);

        auto j4 = json::serialize(
            std::map<std::string, inner_t>{{"inner", inner_t{}}});
        REQUIRE(j4.type() == json11::Json::OBJECT);
        REQUIRE(j4.object_items().size() == 1);

        auto j5 = json::serialize(
            std::unordered_map<std::string, inner_t>{{"inner", inner_t{}}});
        REQUIRE(j5.type() == json11::Json::OBJECT);
        REQUIRE(j5.object_items().size() == 1);
    }

    SECTION("from std containers")
    {
        auto j1 = R"([{"d":2,"r":648}])"_json;

        auto vec = json::deserialize<std::vector<inner_t>>(j1);
        REQUIRE(vec.size() == 1);
        REQUIRE(vec[0].d == Approx(2));
        REQUIRE(vec[0].r == 648);

        auto list = json::deserialize<std::list<inner_t>>(j1);
        REQUIRE(list.size() == 1);
        REQUIRE(list.back().d == Approx(2));
        REQUIRE(list.back().r == 648);

        auto deq = json::deserialize<std::deque<inner_t>>(j1);
        REQUIRE(deq.size() == 1);
        REQUIRE(deq.back().d == Approx(2));
        REQUIRE(deq.back().r == 648);

        auto j2 = R"({"inner": {"d":3,"r":3648}})"_json;

        auto map = json::deserialize<std::map<std::string, inner_t>>(j2);
        REQUIRE(map.count("inner") == 1);
        REQUIRE(map["inner"].d == Approx(3));
        REQUIRE(map["inner"].r == 3648);

        auto umap =
            json::deserialize<std::unordered_map<std::string, inner_t>>(j2);
        REQUIRE(umap.count("inner") == 1);
        REQUIRE(umap["inner"].d == Approx(3));
        REQUIRE(umap["inner"].r == 3648);
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
        return our_type(kl::json::deserialize<int>(json["i"]),
                        kl::json::deserialize<double>(json["d"]),
                        kl::json::deserialize<std::string>(json["str"]));
    }

    json11::Json to_json() const
    {
        return json11::Json::object{{"i", i}, {"d", d}, {"str", str}};
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

namespace kl {
namespace json {

template <>
struct serializer<std::chrono::seconds>
{
    static json11::Json to_json(const std::chrono::seconds& t)
    {
        return {static_cast<double>(t.count())};
    }

    static std::chrono::seconds from_json(const json11::Json& json)
    {
        return std::chrono::seconds{json::deserialize<unsigned>(json)};
    }
};
} // namespace json
} // namespace kl

TEST_CASE("json_convert - extended")
{
    using namespace std::chrono;
    chrono_test t{2, seconds{10}, {seconds{10}, seconds{10}}, our_type{}};
    auto j = kl::json::serialize(t);
    auto obj = kl::json::deserialize<chrono_test>(j);
}

struct global_struct {};
json11::Json to_json(global_struct)
{
    return {"global_struct"};
}

global_struct from_json(kl::type_t<global_struct>, const json11::Json& json)
{
    return json.string_value() == "global_struct"
               ? global_struct{}
               : throw kl::json::deserialize_error{""};
}

namespace {

struct struct_in_anonymous_ns{};
json11::Json to_json(struct_in_anonymous_ns)
{
    return {1};
}

struct_in_anonymous_ns from_json(kl::type_t<struct_in_anonymous_ns>,
                                 const json11::Json&)
{
    return {};
}
} // namespace

namespace my {

struct none_t {};

json11::Json to_json(none_t)
{
    return {nullptr};
}

none_t from_json(kl::type_t<none_t>, const json11::Json& json)
{
    return json.is_null() ? none_t{} : throw kl::json::deserialize_error{""};
}

template <typename T>
struct value_wrapper
{
    T value;
};

// Defining such function with specializaton would not be possible as there's no
// way to partially specialize a function template.
template <typename T>
json11::Json to_json(const value_wrapper<T>& t)
{
    return {static_cast<double>(t.value)};
}

template <typename T>
value_wrapper<T> from_json(kl::type_t<value_wrapper<T>>,
                           const json11::Json& json)
{
    return value_wrapper<T>{kl::json::deserialize<T>(json)};
}
} // namespace my

struct aggregate
{
    global_struct g;
    my::none_t n;
    my::value_wrapper<int> w;
    struct_in_anonymous_ns a;
};
KL_DEFINE_REFLECTABLE(aggregate, (g, /*n,*/ w, a))

TEST_CASE("json_convert - overloading")
{
    aggregate a{{}, {}, {31}, {}};
    auto j = kl::json::serialize(a);
    auto obj = kl::json::deserialize<aggregate>(j);
    REQUIRE(obj.w.value == 31);
}

namespace {

enum class device_type
{
    default_ = (1 << 0),
    cpu = (1 << 1),
    gpu = (1 << 2),
    accelerator = (1 << 3),
    custom = (1 << 4)
};
using device_flags = kl::enum_flags<device_type>;
} // namespace

KL_DEFINE_ENUM_REFLECTOR(device_type, (
    (default_, default),
    cpu,
    gpu,
    accelerator,
    custom
))

TEST_CASE("json_convert - enum_flags")
{
    SECTION("to json")
    {
        auto f = kl::make_flags(device_type::cpu) | device_type::gpu |
                 device_type::accelerator;
        auto j = kl::json::serialize(f);
        REQUIRE(j.is_array());
        REQUIRE(j.array_items().size() == 3);
        REQUIRE(j.array_items()[0] == "cpu");
        REQUIRE(j.array_items()[1] == "gpu");
        REQUIRE(j.array_items()[2] == "accelerator");

        f &= ~kl::make_flags(device_type::accelerator);

        j = kl::json::serialize(f);
        REQUIRE(j.is_array());
        REQUIRE(j.array_items().size() == 2);
        REQUIRE(j.array_items()[0] == "cpu");
        REQUIRE(j.array_items()[1] == "gpu");
    }

    SECTION("from json")
    {
        auto j = R"([])"_json;
        auto f = kl::json::deserialize<device_flags>(j);
        REQUIRE(f.underlying_value() == 0);

        j = R"(["cpu", "gpu"])"_json;
        f = kl::json::deserialize<device_flags>(j);

        REQUIRE(f.underlying_value() ==
                (kl::underlying_cast(device_type::cpu) |
                 kl::underlying_cast(device_type::gpu)));
    }
}
