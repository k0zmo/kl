#include "kl/yaml.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_set.hpp"
#include "input/typedefs.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <tuple>
#include <deque>
#include <list>
#include <map>
#include <optional>
#include <string_view>

TEST_CASE("yaml")
{
    using namespace kl;

    SECTION("serialize basic types")
    {
        CHECK(yaml::serialize('a').as<char>() == 'a');
        CHECK(yaml::serialize(1).as<int>() == 1);
        CHECK(yaml::serialize(3U).as<unsigned>() == 3U);
        CHECK(yaml::serialize(std::int64_t{-1}).as<std::int64_t>() == -1);
        CHECK(yaml::serialize(std::uint64_t{1}).as<std::uint64_t>() == 1);
        CHECK(yaml::serialize(true).as<bool>() == true);
        CHECK(yaml::serialize("qwe").as<std::string>() == "qwe");
        CHECK(yaml::serialize(std::string{"qwe"}).as<std::string>() == "qwe");
        CHECK(yaml::serialize(13.11).as<double>() == Approx(13.11));
        CHECK(yaml::serialize(ordinary_enum::oe_one).as<int>() == 0);
        CHECK(yaml::serialize(nullptr).IsNull());

        const char* qwe = "qwe";
        CHECK(yaml::serialize(qwe).as<std::string>() == "qwe");
    }

    SECTION("deserialize basic types")
    {
        CHECK(yaml::deserialize<int>("-1"_yaml) == -1);
        CHECK(yaml::deserialize<std::string>("\"string\""_yaml) == "string");
        CHECK(yaml::deserialize<unsigned>("33"_yaml) == 33U);
        CHECK(yaml::deserialize<bool>("true"_yaml));
        CHECK(yaml::deserialize<double>("13.11"_yaml) == Approx{13.11});
        CHECK(yaml::deserialize<ordinary_enum>("0"_yaml) ==
              ordinary_enum::oe_one);
    }

    SECTION("parse error")
    {
        REQUIRE_NOTHROW(R"([])"_yaml);
        REQUIRE_THROWS_AS(R"([{]})"_yaml, yaml::parse_error);
    }

    SECTION("serialize inner_t")
    {
        auto y = yaml::serialize(inner_t{});
        REQUIRE(y.IsMap());

        REQUIRE(y["r"]);
        CHECK(y["r"].as<int>() == 1337);
        REQUIRE(y["d"]);
        CHECK(y["d"].as<double>() == Approx(3.145926));
    }

    SECTION("deserialize inner_t - empty yaml")
    {
        REQUIRE_THROWS_AS(yaml::deserialize<inner_t>({}),
                          yaml::deserialize_error);
        REQUIRE_THROWS_WITH(yaml::deserialize<inner_t>({}),
                            "type must be a sequence or map but is a Null\n"
                            "error when deserializing type " +
                                kl::ctti::name<inner_t>());
    }

    SECTION("deserialize inner_t - missing one field")
    {
        auto y = "d: 1.0"_yaml;
        REQUIRE_THROWS_AS(yaml::deserialize<inner_t>(y),
                          yaml::deserialize_error);
        REQUIRE_THROWS_WITH(yaml::deserialize<inner_t>(y),
                            "type must be a scalar but is a Null\n"
                            "error when deserializing field r\n"
                            "error when deserializing type " +
                                kl::ctti::name<inner_t>());
    }

    SECTION("deserialize inner_t - one additional field")
    {
        auto y = "d: 1.0\nr: 2\nzzz: ~"_yaml;
        auto obj = yaml::deserialize<inner_t>(y);
        REQUIRE(obj.r == 2);
        REQUIRE(obj.d == 1.0);
    }

    SECTION("deserialize inner_t")
    {
        auto y = "d: 1.0\nr: 2"_yaml;
        auto obj = yaml::deserialize<inner_t>(y);
        REQUIRE(obj.r == 2);
        REQUIRE(obj.d == 1.0);
    }

    SECTION("deserialize to string_view - invalid type")
    {
        auto j = R"([123])"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<std::string_view>(j),
                            "type must be a scalar but is a Sequence");
    }

    SECTION("unsafe: deserialize to string_view")
    {
        auto j = R"(abc)"_yaml;
        // Str can dangle if `j` is freed before
        auto str = yaml::deserialize<std::string_view>(j);
        REQUIRE(str == "abc");
    }

    SECTION("serialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, true);
        auto y = yaml::serialize(t);

        REQUIRE(y.IsSequence());

        REQUIRE(y.size() == 4);
        CHECK(y[0].as<int>() == 13);
        CHECK(y[1].as<double>() == Approx(3.14));
        CHECK(y[2].as<std::string>() == "lab");
        CHECK(y[3].as<bool>() == true);
    }

    SECTION("deserialize simple - wrong types")
    {
        YAML::Node null{};
        REQUIRE_THROWS_WITH(yaml::deserialize<int>(null),
                            "type must be a scalar but is a Null");
        REQUIRE_THROWS_WITH(yaml::deserialize<bool>(null),
                            "type must be a scalar but is a Null");
        REQUIRE_THROWS_WITH(yaml::deserialize<float>(null),
                            "type must be a scalar but is a Null");
        REQUIRE_THROWS_WITH(yaml::deserialize<std::string>(null),
                            "type must be a scalar but is a Null");
        REQUIRE_THROWS_WITH(yaml::deserialize<std::tuple<int>>(null),
                            "type must be a sequence but is a Null");
        REQUIRE_THROWS_WITH(yaml::deserialize<std::vector<int>>(null),
                            "type must be a sequence but is a Null");
        REQUIRE_THROWS_WITH(
            (yaml::deserialize<std::map<std::string, int>>(null)),
            "type must be a map but is a Null");

        YAML::Node str{"text"};
        REQUIRE_THROWS_WITH(yaml::deserialize<int>(str), "bad conversion");
        REQUIRE_THROWS_WITH(yaml::deserialize<bool>(str), "bad conversion");
        REQUIRE_THROWS_WITH(yaml::deserialize<float>(str), "bad conversion");
        REQUIRE_NOTHROW(yaml::deserialize<std::string>(str));

        YAML::Node arr{YAML::NodeType::Sequence};
        arr.push_back(true);
        REQUIRE_THROWS_WITH(yaml::deserialize<std::vector<int>>(arr),
                            "bad conversion\n"
                            "error when deserializing element 0");

        YAML::Node obj{YAML::NodeType::Map};
        obj["key0"] = YAML::Node{3};
        obj["key2"] = YAML::Node{true};
        REQUIRE_THROWS_WITH(
            (yaml::deserialize<std::map<std::string, int>>(obj)),
            "bad conversion\n"
            "error when deserializing field key2");
    }

    SECTION("deserialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, false);
        auto y = yaml::serialize(t);

        auto obj = yaml::deserialize<decltype(t)>(y);
        REQUIRE(std::get<0>(obj) == 13);
        REQUIRE(std::get<1>(obj) == 3.14);
        REQUIRE(std::get<2>(obj) == colour_space::lab);
        REQUIRE(std::get<3>(obj) == false);

        y = "[7, 13, rgb, true]"_yaml;
        obj = yaml::deserialize<decltype(t)>(y);
        REQUIRE(std::get<0>(obj) == 7);
        REQUIRE(std::get<1>(obj) == 13.0);
        REQUIRE(std::get<2>(obj) == colour_space::rgb);
        REQUIRE(std::get<3>(obj) == true);

        y = R"([7, 13, hls])"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<decltype(t)>(y),
                            "type must be a scalar but is a Null");

        y = "7, 13, rgb, 1, true"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<decltype(t)>(y),
                            "type must be a sequence but is a Scalar");
    }

    SECTION("serialize different types and 'modes' for enums")
    {
        auto y = yaml::serialize(enums{});
        REQUIRE(y["e0"].as<int>() == 0);
        REQUIRE(y["e1"].as<int>() == 0);
        REQUIRE(y["e2"].as<std::string>() == "oe_one_ref");
        REQUIRE(y["e3"].as<std::string>() == "one");
    }

    SECTION("deserialize different types and 'modes' for enums")
    {
        auto y = "{e0: 0, e1: 0, e2: oe_one_ref, e3: one}"_yaml;

        auto obj = yaml::deserialize<enums>(y);
        REQUIRE(obj.e0 == ordinary_enum::oe_one);
        REQUIRE(obj.e1 == scope_enum::one);
        REQUIRE(obj.e2 == ordinary_enum_reflectable::oe_one_ref);
        REQUIRE(obj.e3 == scope_enum_reflectable::one);
    }

    SECTION("deserialize different types and 'modes' for enums - fail")
    {
        auto y = "{e0: 0, e1: 0, e2: oe_one_ref, e3: 0}"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<enums>(y),
                            "invalid enum value: 0\n"
                            "error when deserializing field e3\n"
                            "error when deserializing type " +
                                kl::ctti::name<enums>());

        y = "{e0: 0, e1: 0, e2: oe_one_ref2, e3: 0}"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<enums>(y),
                            "invalid enum value: oe_one_ref2\n"
                            "error when deserializing field e2\n"
                            "error when deserializing type " +
                                kl::ctti::name<enums>());

        y = "{e0: 0, e1: true, e2: oe_one_ref, e3: one}"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<enums>(y),
            "yaml-cpp: error at line 1, column 13: bad conversion\n"
            "error when deserializing field e1\n"
            "error when deserializing type " +
                kl::ctti::name<enums>());

        y = "{e0: 0, e1: 0, e2: oe_one_ref, e3: []}"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<enums>(y),
                            "type must be a scalar but is a Sequence\n"
                            "error when deserializing field e3\n"
                            "error when deserializing type " +
                                kl::ctti::name<enums>());
    }

    SECTION("skip serializing optional fields")
    {
        optional_test t;
        t.non_opt = 23;

        REQUIRE(yaml::serialize(t).IsMap());
        REQUIRE(yaml::serialize(t).size() == 1);
        REQUIRE(yaml::serialize(t)["non_opt"].as<int>() == 23);

        t.opt = 78;
        REQUIRE(yaml::serialize(t).size() == 2);
        REQUIRE(yaml::serialize(t)["non_opt"].as<int>() == 23);
        REQUIRE(yaml::serialize(t)["opt"].as<int>() == 78);
    }

    SECTION("don't skip optional fields if requested")
    {
        optional_test t;
        t.non_opt = 23;

        yaml::serialize_context ctx{false};

        REQUIRE(yaml::serialize(t, ctx).IsMap());
        REQUIRE(yaml::serialize(t, ctx).size() == 2);
        REQUIRE(yaml::serialize(t, ctx)["non_opt"].as<int>() == 23);
        REQUIRE(yaml::serialize(t, ctx)["opt"].IsNull());

        t.opt = 78;
        REQUIRE(yaml::serialize(t, ctx).IsMap());
        REQUIRE(yaml::serialize(t, ctx).size() == 2);
        REQUIRE(yaml::serialize(t, ctx)["non_opt"].as<int>() == 23);
        REQUIRE(yaml::serialize(t, ctx)["opt"].as<int>() == 78);
    }

    SECTION("deserialize fields with null")
    {
        auto y = "{opt: null, non_opt: 3}"_yaml;
        auto obj = yaml::deserialize<optional_test>(y);
        REQUIRE(!obj.opt);
        REQUIRE(obj.non_opt == 3);

        y = "{opt: 4, non_opt: 13}"_yaml;
        obj = yaml::deserialize<optional_test>(y);
        REQUIRE(obj.opt);
        REQUIRE(*obj.opt == 4);
        REQUIRE(obj.non_opt == 13);
    }

    SECTION("deserialize with optional fields missing")
    {
        auto y = "non_opt: 32"_yaml;
        auto obj = yaml::deserialize<optional_test>(y);
        REQUIRE(obj.non_opt == 32);
        REQUIRE(!obj.opt);
    }

    SECTION("deserialize with optional fields invalid")
    {
        auto y = "{non_opt: 32, opt: QWE}"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<optional_test>(y),
            "yaml-cpp: error at line 1, column 20: bad conversion\n"
            "error when deserializing field opt\n"
            "error when deserializing type " +
                kl::ctti::name<optional_test>());
    }

    SECTION("serialize complex structure with std/boost containers")
    {
        test_t t;
        auto y = yaml::serialize(t);
        REQUIRE(y.IsMap());

        REQUIRE(y["hello"].as<std::string>() == "world");
        REQUIRE(y["t"].as<bool>() == true);
        REQUIRE(y["f"].as<bool>() == false);
        REQUIRE(!y["n"]);
        REQUIRE(y["i"].as<int>() == 123);
        REQUIRE(y["pi"].as<double>() == Approx(3.1416f));
        REQUIRE(y["space"].as<std::string>() == "lab");

        REQUIRE(y["a"].IsSequence());
        const auto& a = y["a"];
        REQUIRE(a.size() == 4);
        REQUIRE(a[0].as<int>() == 1);
        REQUIRE(a[3].as<int>() == 4);

        REQUIRE(y["ad"].IsSequence());
        const auto& ad = y["ad"];
        REQUIRE(ad.size() == 2);
        REQUIRE(ad[0].IsSequence());
        REQUIRE(ad[1].IsSequence());
        REQUIRE(ad[0].size() == 2);
        REQUIRE(ad[0].as<YAML::Node>()[0].as<int>() == 1);
        REQUIRE(ad[0].as<YAML::Node>()[1].as<int>() == 2);
        REQUIRE(ad[1].size() == 3);
        REQUIRE(ad[1].as<YAML::Node>()[0].as<int>() == 3);
        REQUIRE(ad[1].as<YAML::Node>()[1].as<int>() == 4);
        REQUIRE(ad[1].as<YAML::Node>()[2].as<int>() == 5);

        REQUIRE(y["tup"].IsSequence());
        REQUIRE(y["tup"].size() == 3);
        REQUIRE(y["tup"].as<YAML::Node>()[0].as<int>() == 1);
        REQUIRE(y["tup"].as<YAML::Node>()[1].as<double>() == Approx{3.14f});
        REQUIRE(y["tup"].as<YAML::Node>()[2].as<std::string>() == "QWE");

        REQUIRE(y["map"].IsMap());
        REQUIRE(y["map"].as<YAML::Node>()["1"].as<std::string>() == "hls");
        REQUIRE(y["map"].as<YAML::Node>()["2"].as<std::string>() == "rgb");

        REQUIRE(y["inner"].IsMap());
        const auto& inner = y["inner"].as<YAML::Node>();
        REQUIRE(inner["r"].as<int>() == inner_t{}.r);
        REQUIRE(inner["d"].as<double>() == inner_t{}.d);
    }

    SECTION("deserialize complex structure")
    {
        auto y = R"(
a:
  - 10
  - 20
  - 30
  - 40
ad:
  -
    - 20
  -
    - 30
    - 40
    - 50
f: true
hello: new world
i: 456
inner:
  d: 2.71
  r: 667
map:
  10: xyz
  20: lab
n: 3
pi: 3.1416
space: rgb
t: false
tup:
  - 10
  - 31.4
  - ASD
)"_yaml;

        auto obj = yaml::deserialize<test_t>(y);

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
        REQUIRE(obj.pi == 3.1416f);
        REQUIRE(obj.space == colour_space::rgb);
        REQUIRE(obj.t == false);
        using namespace std::string_literals;
        REQUIRE(obj.tup == std::make_tuple(10, 31.4, "ASD"s));
    }

    SECTION("test unsigned types")
    {
        unsigned_test t;
        auto y = yaml::serialize(t);

        REQUIRE(y["u8"].as<unsigned char>() == t.u8);
        REQUIRE(y["u16"].as<unsigned short>() == t.u16);
        REQUIRE(y["u32"].as<unsigned int>() == t.u32);
        REQUIRE(y["u64"].as<std::uint64_t>() == t.u64);

        auto obj = yaml::deserialize<unsigned_test>(y);
        REQUIRE(obj.u8 == t.u8);
        REQUIRE(obj.u16 == t.u16);
        REQUIRE(obj.u32 == t.u32);
        REQUIRE(obj.u64 == t.u64);
    }

    SECTION("try to deserialize too big value to (u)intX_t")
    {
        auto y = R"(500)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint8_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(200000000000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint8_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(70000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint16_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-70000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint32_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-70000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint64_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(200000000000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint32_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(9022337203623423400234234234234854775807)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint32_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(9022337203623423400234234234234854775807)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::uint64_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-500)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::int8_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-70000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::int16_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-200000000000)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::int32_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-9022337203623423400234234234234854775807)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::int32_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(-9022337203623423400234234234234854775807)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::int64_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");

        y = R"(9443372036854775807)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<std::int64_t>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");
    }

    SECTION("deserialize to struct from an array")
    {
        auto y = "[3,4.0]"_yaml;
        auto obj = yaml::deserialize<inner_t>(y);
        REQUIRE(obj.r == 3);
        REQUIRE(obj.d == 4.0);
    }

    SECTION("deserialize to struct from an array - num elements differs")
    {
        auto y = "[3,4.0,QWE]"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<inner_t>(y),
            "sequence size is greater than declared struct's field count\n"
            "error when deserializing type " +
                kl::ctti::name<inner_t>());

        y = "- 3"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<inner_t>(y),
                            "type must be a scalar but is a Null\n"
                            "error when deserializing element 1\n"
                            "error when deserializing type " +
                                kl::ctti::name<inner_t>());
    }

    SECTION("deserialize to struct from an array - tail optional fields")
    {
        auto y = "- 234"_yaml;
        auto obj = yaml::deserialize<optional_test>(y);
        REQUIRE(obj.non_opt == 234);
        REQUIRE(!obj.opt);
    }

    SECTION("deserialize floating-point number to int")
    {
        auto y = R"(3.0)"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<int>(y),
            "yaml-cpp: error at line 1, column 1: bad conversion");
    }

    SECTION("deserialize to struct from an array - type mismatch")
    {
        auto y = "[3,QWE]"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<inner_t>(y),
            "yaml-cpp: error at line 1, column 4: bad conversion\n"
            "error when deserializing element 1\n"
            "error when deserializing type " +
                kl::ctti::name<inner_t>());

        y = "[false,4]"_yaml;
        REQUIRE_THROWS_WITH(
            yaml::deserialize<inner_t>(y),
            "yaml-cpp: error at line 1, column 2: bad conversion\n"
            "error when deserializing element 0\n"
            "error when deserializing type " +
                kl::ctti::name<inner_t>());
    }

    SECTION("optional<string>")
    {
        auto y = "~"_yaml;
        auto a = yaml::deserialize<std::optional<std::string>>(y);
        REQUIRE(!a);

        y = "asd"_yaml;
        auto b = yaml::deserialize<std::optional<std::string>>(y);
        REQUIRE(b);
        REQUIRE(b.value() == "asd");
    }

    SECTION("tuple with tail optionals")
    {
        using tuple_t = std::tuple<int, bool, std::optional<std::string>>;

        auto y = "[4]"_yaml;
        REQUIRE_THROWS_WITH(yaml::deserialize<tuple_t>(y),
                            "type must be a scalar but is a Null");

        y = "[4,true]"_yaml;
        auto t = yaml::deserialize<tuple_t>(y);
        REQUIRE(std::get<0>(t) == 4);
        REQUIRE(std::get<1>(t));
        REQUIRE(!std::get<2>(t).has_value());
    }

    SECTION("to std containers")
    {
        auto y1 = yaml::serialize(std::vector<inner_t>{inner_t{}});
        REQUIRE(y1.IsSequence());
        REQUIRE(y1.size() == 1);

        auto y2 = yaml::serialize(std::list<inner_t>{inner_t{}});
        REQUIRE(y2.IsSequence());
        REQUIRE(y2.size() == 1);

        auto y3 = yaml::serialize(std::deque<inner_t>{inner_t{}});
        REQUIRE(y3.IsSequence());
        REQUIRE(y3.size() == 1);

        auto y4 = yaml::serialize(
            std::map<std::string, inner_t>{{"inner", inner_t{}}});
        REQUIRE(y4.IsMap());
        REQUIRE(y4.size() == 1);

        auto y5 = yaml::serialize(
            std::unordered_map<std::string, inner_t>{{"inner", inner_t{}}});
        REQUIRE(y5.IsMap());
        REQUIRE(y5.size() == 1);
    }

    SECTION("from std containers")
    {
        auto y1 = "- d: 2\n  r: 648"_yaml;

        auto vec = yaml::deserialize<std::vector<inner_t>>(y1);
        REQUIRE(vec.size() == 1);
        REQUIRE(vec[0].d == Approx(2));
        REQUIRE(vec[0].r == 648);

        auto list = yaml::deserialize<std::list<inner_t>>(y1);
        REQUIRE(list.size() == 1);
        REQUIRE(list.back().d == Approx(2));
        REQUIRE(list.back().r == 648);

        auto deq = yaml::deserialize<std::deque<inner_t>>(y1);
        REQUIRE(deq.size() == 1);
        REQUIRE(deq.back().d == Approx(2));
        REQUIRE(deq.back().r == 648);

        auto y2 = R"({inner: {d: 3,r: 3648}})"_yaml;

        auto map = yaml::deserialize<std::map<std::string, inner_t>>(y2);
        REQUIRE(map.count("inner") == 1);
        REQUIRE(map["inner"].d == Approx(3));
        REQUIRE(map["inner"].r == 3648);

        auto umap =
            yaml::deserialize<std::unordered_map<std::string, inner_t>>(y2);
        REQUIRE(umap.count("inner") == 1);
        REQUIRE(umap["inner"].d == Approx(3));
        REQUIRE(umap["inner"].r == 3648);
    }
}

namespace kl {
namespace yaml {

template <>
struct serializer<std::chrono::seconds>
{
    template <typename Context>
    static YAML::Node to_yaml(const std::chrono::seconds& t, Context& ctx)
    {
        return yaml::serialize(t.count(), ctx);
    }

    static void from_yaml(std::chrono::seconds& out, const YAML::Node& value)
    {
        out = std::chrono::seconds{yaml::deserialize<long long>(value)};
    }
};
} // namespace yaml
} // namespace kl

TEST_CASE("yaml - extended")
{
    using namespace std::chrono;
    chrono_test t{2, seconds{10}, {seconds{10}, seconds{10}}};
    auto y = kl::yaml::serialize(t);
    auto obj = kl::yaml::deserialize<chrono_test>(y);
}

template <typename Context>
YAML::Node to_yaml(global_struct, Context& ctx)
{
    return kl::yaml::serialize("global_struct", ctx);
}

void from_yaml(global_struct& out, const YAML::Node& value)
{
    if (value.Scalar() != "global_struct")
        throw kl::yaml::deserialize_error{""};
    out = global_struct{};
}

namespace my {

template <typename Context>
YAML::Node to_yaml(none_t, Context&)
{
    return YAML::Node{};
}

void from_yaml(none_t& out, const YAML::Node& value)
{
    out = value.IsNull() ? none_t{} : throw kl::yaml::deserialize_error{""};
}

// Defining such function with specializaton would not be possible as there's no
// way to partially specialize a function template.
template <typename T, typename Context>
YAML::Node to_yaml(const value_wrapper<T>& t, Context& ctx)
{
    return kl::yaml::serialize(t.value, ctx);
}

template <typename T>
void from_yaml(value_wrapper<T>& out, const YAML::Node& value)
{
    out = value_wrapper<T>{kl::yaml::deserialize<T>(value)};
}
} // namespace my

TEST_CASE("yaml - overloading")
{
    aggregate a{{}, {}, {31}};
    auto y = kl::yaml::serialize(a);
    REQUIRE(y["n"]);
    CHECK(y["n"].IsNull());
    auto obj = kl::yaml::deserialize<aggregate>(y);
    REQUIRE(obj.w.value == 31);
}

TEST_CASE("yaml - enum_set")
{
    SECTION("to yaml")
    {
        auto f = kl::enum_set{device_type::cpu} | device_type::gpu |
                 device_type::accelerator;
        auto y = kl::yaml::serialize(f);
        REQUIRE(y.IsSequence());
        REQUIRE(y.size() == 3);
        REQUIRE(y[0].as<std::string>() == "cpu");
        REQUIRE(y[1].as<std::string>() == "gpu");
        REQUIRE(y[2].as<std::string>() == "accelerator");

        f &= ~kl::enum_set{device_type::accelerator};

        y = kl::yaml::serialize(f);
        REQUIRE(y.IsSequence());
        REQUIRE(y.size() == 2);
        REQUIRE(y[0].as<std::string>() == "cpu");
        REQUIRE(y[1].as<std::string>() == "gpu");
    }

    SECTION("from yaml")
    {
        auto y = "cpu: 1"_yaml;
        REQUIRE_THROWS_WITH(kl::yaml::deserialize<device_flags>(y),
                            "type must be a sequence but is a Map");

        y = "[]"_yaml;
        auto f = kl::yaml::deserialize<device_flags>(y);
        REQUIRE(f.underlying_value() == 0);

        y = "[cpu, gpu]"_yaml;
        f = kl::yaml::deserialize<device_flags>(y);

        REQUIRE(f.underlying_value() ==
                (kl::underlying_cast(device_type::cpu) |
                 kl::underlying_cast(device_type::gpu)));
    }
}

TEST_CASE("yaml dump")
{
    using namespace kl;

    SECTION("basic types")
    {
        CHECK(yaml::dump('a') == "a");
        CHECK(yaml::dump(1) == "1");
        CHECK(yaml::dump(3U) == "3");
        CHECK(yaml::dump(std::int64_t{-1}) == "-1");
        CHECK(yaml::dump(std::uint64_t{1}) == "1");
        CHECK(yaml::dump(true) == "true");
        CHECK(yaml::dump(nullptr) == "~");
        CHECK(yaml::dump("qwe") == "qwe");
        CHECK(yaml::dump(std::string{"qwe"}) == "qwe");
        CHECK(yaml::dump(13.11) == "13.11");
        CHECK(yaml::dump(ordinary_enum::oe_one) == "0");
    }

    SECTION("inner_t")
    {
        CHECK(yaml::dump(inner_t{}) == "r: 1337\nd: 3.145926");
    }

    SECTION("different types and 'modes' for enums")
    {
        CHECK(yaml::dump(enums{}) == "e0: 0\ne1: 0\ne2: oe_one_ref\ne3: one");
    }

    SECTION("test unsigned types")
    {
        auto res = yaml::dump(unsigned_test{});
        CHECK(res == "u8: \"\\x80\"\nu16: 32768\nu32: 4294967295\nu64: "
                     "18446744073709551615");
    }

    SECTION("enum_set")
    {
        auto f = kl::enum_set{device_type::cpu} | device_type::gpu |
                 device_type::accelerator;
        auto res = yaml::dump(f);
        CHECK(res == "- cpu\n- gpu\n- accelerator");

        f &= ~kl::enum_set{device_type::accelerator};
        res = yaml::dump(f);
        CHECK(res == "- cpu\n- gpu");
    }

    SECTION("tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, true);
        auto res = yaml::dump(t);
        CHECK(res == "- 13\n- 3.14\n- lab\n- true");
    }

    SECTION("skip serializing optional fields")
    {
        optional_test t;
        t.non_opt = 23;

        CHECK(yaml::dump(t) == "non_opt: 23");

        t.opt = 78;
        CHECK(yaml::dump(t) == "non_opt: 23\nopt: 78");
    }

    SECTION("don't skip optional fields if requested")
    {
        optional_test t;
        t.non_opt = 23;

        YAML::Emitter emitter;
        yaml::dump_context ctx{emitter, false};

        yaml::dump(t, ctx);
        std::string res = emitter.c_str();
        CHECK(res == "non_opt: 23\nopt: ~");

        YAML::Emitter emitter2;
        yaml::dump_context ctx2{emitter2, false};

        t.opt = 78;
        yaml::dump(t, ctx2);
        res = emitter2.c_str();
        CHECK(res == "non_opt: 23\nopt: 78");
    }

    SECTION("complex structure with std/boost containers")
    {
        auto res = yaml::dump(test_t{});
        CHECK(res ==
              R"(hello: world
t: true
f: false
i: 123
pi: 3.1416
a:
  - 1
  - 2
  - 3
  - 4
ad:
  -
    - 1
    - 2
  -
    - 3
    - 4
    - 5
space: lab
tup:
  - 1
  - 3.140000104904175
  - QWE
map:
  1: hls
  2: rgb
inner:
  r: 1337
  d: 3.145926)");
    }

    SECTION("unsigned: check value greater than 0x7FFFFFFU")
    {
        REQUIRE(yaml::dump(2147483648U) == "2147483648");
    }

    SECTION("std containers")
    {
        auto y1 = yaml::dump(std::vector<inner_t>{inner_t{}});
        CHECK(y1 == "- r: 1337\n  d: 3.145926");

        auto y2 = yaml::dump(std::list<inner_t>{inner_t{}});
        CHECK(y2 == "- r: 1337\n  d: 3.145926");

        auto y3 = yaml::dump(std::deque<inner_t>{inner_t{}});
        CHECK(y3 == "- r: 1337\n  d: 3.145926");

        auto y4 =
            yaml::dump(std::map<std::string, inner_t>{{"inner1", inner_t{}}});
        CHECK(y4 == "inner1:\n  r: 1337\n  d: 3.145926");

        auto y5 = yaml::dump(
            std::unordered_map<std::string, inner_t>{{"inner2", inner_t{}}});
        CHECK(y5 == "inner2:\n  r: 1337\n  d: 3.145926");
    }
}

namespace {

class my_dump_context
{
public:
    explicit my_dump_context(YAML::Emitter& emitter) : emitter_{emitter} {}

    YAML::Emitter& emitter() const { return emitter_; }

    template <typename Key, typename Value>
    bool skip_field(const Key& key, const Value& value)
    {
        return !std::strcmp(key, "secret");
    }

private:
    YAML::Emitter& emitter_;
};
} // namespace

TEST_CASE("yaml dump - custom context")
{
    using namespace kl;

    YAML::Emitter emitter;
    my_dump_context ctx{emitter};

    yaml::dump(struct_with_blacklisted{}, ctx);
    std::string res = emitter.c_str();
    CHECK(res == "value: 34\nother_non_secret: true");
}

namespace kl {
namespace yaml {

template <>
struct encoder<std::chrono::seconds>
{
    template <typename Context>
    static void encode(const std::chrono::seconds& s, Context& ctx)
    {
        yaml::dump(s.count(), ctx);
    }
};
} // namespace yaml
} // namespace kl

TEST_CASE("yaml dump - extended")
{
    using namespace std::chrono;
    chrono_test t{2, seconds{10}, {seconds{6}, seconds{12}}};
    const auto res = kl::yaml::dump(t);
    CHECK(res == "t: 2\nsec: 10\nsecs:\n  - 6\n  - 12");
}

template <typename Context>
void encode(global_struct, Context& ctx)
{
    kl::yaml::dump("global_struct", ctx);
}

namespace my {

template <typename Context>
void encode(none_t, Context& ctx)
{
    kl::yaml::dump(nullptr, ctx);
}

template <typename T, typename Context>
void encode(const value_wrapper<T>& t, Context& ctx)
{
    kl::yaml::dump(t.value, ctx);
}
} // namespace my

TEST_CASE("yaml dump - overloading")
{
    aggregate a{{}, {}, {31}};
    auto res = kl::yaml::dump(a);
    CHECK(res == "g: global_struct\nn: ~\nw: 31");
}

namespace {

enum class event_type
{
    a,
    b,
    c
};
KL_DESCRIBE_ENUM(event_type, a, b, c)

struct event
{
    event_type type;
    kl::yaml::view data;
};
KL_DESCRIBE_FIELDS(event, type, data)

struct event_a
{
    int f1;
    bool f2;
    std::string f3;
};
KL_DESCRIBE_FIELDS(event_a, f1, f2, f3)

using event_c = std::tuple<std::string, bool, std::vector<int>>;
} // namespace

TEST_CASE("yaml::view - two-phase deserialization")
{
    auto y =
        R"(---
- type: a
  data:
    f1: 3
    f2: true
    f3: something
- type: c
  data:
  - d1
  - false
  - - 1
    - 2
    - 3
)"_yaml;

    auto objs = kl::yaml::deserialize<std::vector<event>>(y);
    REQUIRE(objs.size() == 2);
    CHECK(objs[0].type == event_type::a);
    REQUIRE(objs[0].data);
    CHECK(objs[0].data->IsMap());

    auto e1 = kl::yaml::deserialize<event_a>(objs[0].data);
    CHECK(e1.f1 == 3);
    CHECK(e1.f2);
    CHECK(e1.f3 == "something");

    CHECK(objs[1].type == event_type::c);
    REQUIRE(objs[1].data);
    CHECK((*objs[1].data).IsSequence());
    CHECK(objs[1].data->IsSequence());

    auto [a, b, c] = kl::yaml::deserialize<event_c>(objs[1].data);
    CHECK(a == "d1");
    CHECK_FALSE(b);
    CHECK_THAT(c, Catch::Equals<int>({1, 2, 3}));

    CHECK(kl::yaml::dump(objs) == R"(- type: a
  data:
    f1: 3
    f2: true
    f3: something
- type: c
  data:
    - d1
    - false
    -
      - 1
      - 2
      - 3)");
}

namespace {

struct zxc
{
    std::string a;
    int b;
    bool c;
    std::vector<int> d;

    template <typename Context>
    friend YAML::Node to_yaml(const zxc& z, Context& ctx)
    {
        YAML::Node ret{YAML::NodeType::Map};
        ret["a"] = kl::yaml::serialize(z.a, ctx);
        ret["b"] = kl::yaml::serialize(z.b, ctx);
        ret["c"] = kl::yaml::serialize(z.c, ctx);
        ret["d"] = kl::yaml::serialize(z.d, ctx);
        return ret;
    }

    friend void from_yaml(zxc& z, const YAML::Node& value)
    {
        if (!value.IsMap())
        {
            throw kl::yaml::deserialize_error{"type must be a map but is a " +
                                              kl::yaml::type_name(value)};
        }

        kl::yaml::deserialize(z.a, kl::yaml::get_value(value, "a"));
        kl::yaml::deserialize(z.b, kl::yaml::get_value(value, "b"));
        kl::yaml::deserialize(z.c, kl::yaml::get_value(value, "c"));
        kl::yaml::deserialize(z.d, kl::yaml::get_value(value, "d"));
    }
};
} // namespace

TEST_CASE("yaml: manually (de)serialized type")
{
    zxc z{"asd", 3, true, {1, 2, 34}};
    CHECK(YAML::Dump(kl::yaml::serialize(z)) ==
          R"(a: asd
b: 3
c: true
d:
  - 1
  - 2
  - 34)");

    auto zz = kl::yaml::deserialize<zxc>(kl::yaml::serialize(z));

    CHECK(zz.a == z.a);
    CHECK(zz.b == z.b);
    CHECK(zz.c == z.c);
    CHECK(zz.d == z.d);
}
