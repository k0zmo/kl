#include "kl/json.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_flags.hpp"
#include "input/typedefs.hpp"

#include <catch2/catch.hpp>
#include <boost/optional/optional_io.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <tuple>
#include <deque>
#include <list>
#include <map>

std::string to_string(const rapidjson::Document& doc)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return std::string(buffer.GetString());
}

TEST_CASE("json")
{
    using namespace kl;

    SECTION("serialize basic types")
    {
        CHECK(json::serialize('a').IsInt());
        CHECK(json::serialize(1).IsInt());
        CHECK(json::serialize(3U).IsInt());
        CHECK(json::serialize(std::int64_t{-1}).IsInt64());
        CHECK(json::serialize(std::uint64_t{1}).IsUint64());
        CHECK(json::serialize(true).IsBool());
        CHECK(json::serialize("qwe").IsString());
        CHECK(json::serialize(std::string{"qwe"}).IsString());
        CHECK(json::serialize(13.11).IsDouble());
        CHECK(json::serialize(ordinary_enum::oe_one).IsInt());

        const char* qwe = "qwe";
        CHECK(json::serialize(qwe).IsString());
    }

    SECTION("deserialize basic types")
    {
        CHECK(json::deserialize<int>("-1"_json) == -1);
        CHECK(json::deserialize<std::string>("\"string\""_json) == "string");
        CHECK(json::deserialize<unsigned>("33"_json) == 33U);
        CHECK(json::deserialize<bool>("true"_json));
        CHECK(json::deserialize<double>("13.11"_json) == Approx{13.11});
        CHECK(json::deserialize<ordinary_enum>("0"_json) ==
              ordinary_enum::oe_one);
    }

    SECTION("parse error")
    {
        REQUIRE_NOTHROW(R"([])"_json);
        REQUIRE_THROWS_AS(R"([{]})"_json, json::parse_error);
    }

    SECTION("serialize inner_t")
    {
        auto j = json::serialize(inner_t{});
        REQUIRE(j.IsObject());

        auto obj = j.GetObject();
        auto it = obj.FindMember("r");
        REQUIRE(it != obj.end());
        REQUIRE(it->value.GetInt() == 1337);
        it = obj.FindMember("d");
        REQUIRE(it != obj.end());
        REQUIRE(it->value.GetDouble() == Approx(3.145926));
    }

    SECTION("deserialize inner_t - missing one field")
    {
        auto j = R"({"d": 1.0})"_json;
        REQUIRE_THROWS_AS(json::deserialize<inner_t>(j),
                          json::deserialize_error);
        REQUIRE_THROWS_WITH(json::deserialize<inner_t>(j),
                            "type must be an integral but is kNullType\n"
                            "error when deserializing field r\n"
                            "error when deserializing type inner_t");
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
        auto t = std::make_tuple(13, 3.14, colour_space::lab, true);
        auto j = json::serialize(t);

        REQUIRE(j.IsArray());
        auto arr = j.GetArray();

        REQUIRE(arr.Size() == 4);
        REQUIRE(arr[0] == 13);
        REQUIRE(arr[1].GetDouble() == Approx(3.14));
        REQUIRE(arr[2] == "lab");
        REQUIRE(arr[3] == true);
    }

    SECTION("deserialize simple - wrong types")
    {
        rapidjson::Value null{};
        REQUIRE_THROWS_WITH(json::deserialize<int>(null),
                            "type must be an integral but is kNullType");
        REQUIRE_THROWS_WITH(json::deserialize<bool>(null),
                            "type must be a bool but is kNullType");
        REQUIRE_THROWS_WITH(json::deserialize<float>(null),
                            "type must be a floating-point but is kNullType");
        REQUIRE_THROWS_WITH(json::deserialize<std::string>(null),
                            "type must be a string but is kNullType");
        REQUIRE_THROWS_WITH(json::deserialize<std::tuple<int>>(null),
                            "type must be an array but is kNullType");
        REQUIRE_THROWS_WITH(json::deserialize<std::vector<int>>(null),
                            "type must be an array but is kNullType");
        REQUIRE_THROWS_WITH(
            (json::deserialize<std::map<std::string, int>>(null)),
            "type must be an object but is kNullType");

        rapidjson::Value str{"\"text\""};
        REQUIRE_THROWS_WITH(json::deserialize<int>(str),
                            "type must be an integral but is kStringType");
        REQUIRE_THROWS_WITH(json::deserialize<bool>(str),
                            "type must be a bool but is kStringType");
        REQUIRE_THROWS_WITH(json::deserialize<float>(str),
                            "type must be a floating-point but is kStringType");
        REQUIRE_NOTHROW(json::deserialize<std::string>(str));

        rapidjson::Document arr{rapidjson::kArrayType};
        arr.PushBack(true, arr.GetAllocator());
        REQUIRE_THROWS_WITH(json::deserialize<std::vector<int>>(arr),
                            "type must be an integral but is kTrueType\n"
                            "error when deserializing element 0");

        rapidjson::Document obj{rapidjson::kObjectType};
        obj.AddMember("key0", rapidjson::Value{3}, obj.GetAllocator());
        obj.AddMember("key2", rapidjson::Value{true}, obj.GetAllocator());
        REQUIRE_THROWS_WITH(
            (json::deserialize<std::map<std::string, int>>(obj)),
            "type must be an integral but is kTrueType\n"
            "error when deserializing field key2");
    }

    SECTION("deserialize tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, false);
        auto j = json::serialize(t);

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

        j = R"([7, 13, "hls"])"_json;
        REQUIRE_THROWS_WITH(json::deserialize<decltype(t)>(j),
                            "type must be a bool but is kNullType");

        j = R"([7, 13, "rgb", 1, true])"_json;
        REQUIRE_THROWS_WITH(json::deserialize<decltype(t)>(j),
                            "type must be a bool but is kNumberType");
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
        REQUIRE_THROWS_WITH(json::deserialize<enums>(j),
                            "type must be a string-enum but is kNumberType\n"
                            "error when deserializing field e3\n"
                            "error when deserializing type enums");

        j = R"({"e0": 0, "e1": 0, "e2": "oe_one_ref2", "e3": 0})"_json;
        REQUIRE_THROWS_WITH(json::deserialize<enums>(j),
                            "invalid enum value: oe_one_ref2\n"
                            "error when deserializing field e2\n"
                            "error when deserializing type enums");

        j = R"({"e0": 0, "e1": true, "e2": "oe_one_ref", "e3": "one"})"_json;
        REQUIRE_THROWS_WITH(json::deserialize<enums>(j),
                            "type must be a number-enum but is kTrueType\n"
                            "error when deserializing field e1\n"
                            "error when deserializing type enums");
    }

    SECTION("skip serializing optional fields")
    {
        optional_test t;
        t.non_opt = 23;

        REQUIRE(json::serialize(t).IsObject());
        REQUIRE(json::serialize(t).MemberCount() == 1);
        REQUIRE(json::serialize(t)["non_opt"] == 23);

        t.opt = 78;
        REQUIRE(json::serialize(t).MemberCount() == 2);
        REQUIRE(json::serialize(t)["non_opt"] == 23);
        REQUIRE(json::serialize(t)["opt"] == 78);
    }

    SECTION("don't skip optional fields if requested")
    {
        optional_test t;
        t.non_opt = 23;

        rapidjson::Document doc;
        json::serialize_context ctx{doc, false};

        REQUIRE(json::serialize(t, ctx).IsObject());
        REQUIRE(json::serialize(t, ctx).MemberCount() == 2);
        REQUIRE(json::serialize(t, ctx)["non_opt"] == 23);
        REQUIRE(json::serialize(t, ctx)["opt"].IsNull());

        t.opt = 78;
        REQUIRE(json::serialize(t, ctx).IsObject());
        REQUIRE(json::serialize(t, ctx).MemberCount() == 2);
        REQUIRE(json::serialize(t, ctx)["non_opt"] == 23);
        REQUIRE(json::serialize(t, ctx)["opt"] == 78);
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
        REQUIRE_THROWS_WITH(json::deserialize<optional_test>(j),
                            "type must be an integral but is kStringType\n"
                            "error when deserializing field opt\n"
                            "error when deserializing type optional_test");
    }

    SECTION("serialize complex structure with std/boost containers")
    {
        test_t t;
        auto j = json::serialize(t);
        REQUIRE(j.IsObject());
        REQUIRE(j["hello"] == "world");
        REQUIRE(j["t"] == true);
        REQUIRE(j["f"] == false);
        REQUIRE(!j.HasMember("n"));
        REQUIRE(j["i"] == 123);
        REQUIRE(j["pi"] == 3.1416f);
        REQUIRE(j["space"] == "lab");

        REQUIRE(j["a"].IsArray());
        auto a = j["a"].GetArray();
        REQUIRE(a.Size() == 4);
        REQUIRE(a[0] == 1);
        REQUIRE(a[3] == 4);

        REQUIRE(j["ad"].IsArray());
        auto ad = j["ad"].GetArray();
        REQUIRE(ad.Size() == 2);
        REQUIRE(ad[0].IsArray());
        REQUIRE(ad[1].IsArray());
        REQUIRE(ad[0].Size() == 2);
        REQUIRE(ad[0][0] == 1);
        REQUIRE(ad[0][1] == 2);
        REQUIRE(ad[1].Size() == 3);
        REQUIRE(ad[1][0] == 3);
        REQUIRE(ad[1][1] == 4);
        REQUIRE(ad[1][2] == 5);

        REQUIRE(j["tup"].IsArray());
        REQUIRE(j["tup"].Size() == 3);
        REQUIRE(j["tup"][0] == 1);
        REQUIRE(j["tup"][1] == 3.14f);
        REQUIRE(j["tup"][2] == "QWE");

        REQUIRE(j["map"].IsObject());
        REQUIRE(j["map"]["1"] == "hls");
        REQUIRE(j["map"]["2"] == "rgb");

        REQUIRE(j["inner"].IsObject());
        auto inner = j["inner"].GetObject();
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
        REQUIRE(obj.pi == 3.1416f);
        REQUIRE(obj.space == colour_space::rgb);
        REQUIRE(obj.t == false);
        using namespace std::string_literals;
        REQUIRE(obj.tup == std::make_tuple(10, 31.4, "ASD"s));
    }

    SECTION("test unsigned types")
    {
        unsigned_test t;
        auto j = json::serialize(t);

        REQUIRE(j["u8"] == t.u8);
        REQUIRE(j["u16"] == t.u16);
        REQUIRE(j["u32"] == t.u32);
        REQUIRE(j["u64"] == t.u64);

        auto obj = json::deserialize<unsigned_test>(j);
        REQUIRE(obj.u8 == t.u8);
        REQUIRE(obj.u16 == t.u16);
        REQUIRE(obj.u32 == t.u32);
        REQUIRE(obj.u64 == t.u64);
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
        REQUIRE_THROWS_WITH(
            json::deserialize<inner_t>(j),
            "array size is greater than declared struct's field count\n"
            "error when deserializing type inner_t");

        j = R"([3])"_json;
        REQUIRE_THROWS_WITH(json::deserialize<inner_t>(j),
                            "type must be a floating-point but is kNullType\n"
                            "error when deserializing element 1\n"
                            "error when deserializing type inner_t");
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
        REQUIRE_THROWS_WITH(json::deserialize<inner_t>(j),
                            "type must be a floating-point but is kStringType\n"
                            "error when deserializing element 1\n"
                            "error when deserializing type inner_t");

        j = R"([false,4])"_json;
        REQUIRE_THROWS_WITH(json::deserialize<inner_t>(j),
                            "type must be an integral but is kFalseType\n"
                            "error when deserializing element 0\n"
                            "error when deserializing type inner_t");
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
        REQUIRE_THROWS_WITH(json::deserialize<tuple_t>(j),
                            "type must be a bool but is kNullType");

        j = R"([4,true])"_json;
        auto t = json::deserialize<tuple_t>(j);
        REQUIRE(std::get<0>(t) == 4);
        REQUIRE(std::get<1>(t));
        REQUIRE(!std::get<2>(t).is_initialized());
    }

    SECTION("unsigned: check 32")
    {
        auto j = json::serialize(unsigned(32));
        REQUIRE(j.GetDouble() == 32.0);
        REQUIRE(j.GetInt() == 32);
        REQUIRE(to_string(j) == "32");
    }

    SECTION("unsigned: check value greater than 0x7FFFFFFU")
    {
        auto j = json::serialize(2147483648U);
        REQUIRE(j.GetDouble() == 2147483648.0);
        REQUIRE(j.GetUint64() == 2147483648U);
        REQUIRE(to_string(j) == "2147483648");
    }

    SECTION("to std containers")
    {
        auto j1 = json::serialize(std::vector<inner_t>{inner_t{}});
        REQUIRE(j1.IsArray());
        REQUIRE(j1.Size() == 1);

        auto j2 = json::serialize(std::list<inner_t>{inner_t{}});
        REQUIRE(j2.IsArray());
        REQUIRE(j2.Size() == 1);

        auto j3 = json::serialize(std::deque<inner_t>{inner_t{}});
        REQUIRE(j3.IsArray());
        REQUIRE(j3.Size() == 1);

        auto j4 = json::serialize(
            std::map<std::string, inner_t>{{"inner", inner_t{}}});
        REQUIRE(j4.IsObject());
        REQUIRE(j4.MemberCount() == 1);

        auto j5 = json::serialize(
            std::unordered_map<std::string, inner_t>{{"inner", inner_t{}}});
        REQUIRE(j5.IsObject());
        REQUIRE(j5.MemberCount() == 1);
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

namespace kl {
namespace json {

template <>
struct serializer<std::chrono::seconds>
{
    template <typename Context>
    static rapidjson::Value to_json(const std::chrono::seconds& t, Context& ctx)
    {
        return json::serialize(t.count(), ctx);
    }

    static std::chrono::seconds from_json(const rapidjson::Value& value)
    {
        return std::chrono::seconds{json::deserialize<long long>(value)};
    }
};
} // namespace json
} // namespace kl

TEST_CASE("json - extended")
{
    using namespace std::chrono;
    chrono_test t{2, seconds{10}, {seconds{10}, seconds{10}}};
    auto j = kl::json::serialize(t);
    auto obj = kl::json::deserialize<chrono_test>(j);
}

template <typename Context>
rapidjson::Value to_json(global_struct, Context& ctx)
{
    return kl::json::serialize("global_struct", ctx);
}

global_struct from_json(kl::type_t<global_struct>,
                        const rapidjson::Value& value)
{
    return value == "global_struct" ? global_struct{}
                                    : throw kl::json::deserialize_error{""};
}

namespace my {

template <typename Context>
rapidjson::Value to_json(none_t, Context&)
{
    return rapidjson::Value{};
}

none_t from_json(kl::type_t<none_t>, const rapidjson::Value& value)
{
    return value.IsNull() ? none_t{} : throw kl::json::deserialize_error{""};
}

// Defining such function with specializaton would not be possible as there's no
// way to partially specialize a function template.
template <typename T, typename Context>
rapidjson::Value to_json(const value_wrapper<T>& t, Context& ctx)
{
    return kl::json::serialize(t.value, ctx);
}

template <typename T>
value_wrapper<T> from_json(kl::type_t<value_wrapper<T>>,
                           const rapidjson::Value& value)
{
    return value_wrapper<T>{kl::json::deserialize<T>(value)};
}
} // namespace my

TEST_CASE("json - overloading")
{
    aggregate a{{}, {}, {31}};
    auto j = kl::json::serialize(a);
    REQUIRE(j.FindMember("n") != j.MemberEnd());
    CHECK(j["n"].IsNull());
    auto obj = kl::json::deserialize<aggregate>(j);
    REQUIRE(obj.w.value == 31);
}

TEST_CASE("json - enum_flags")
{
    SECTION("to json")
    {
        auto f = kl::make_flags(device_type::cpu) | device_type::gpu |
                 device_type::accelerator;
        auto j = kl::json::serialize(f);
        REQUIRE(j.IsArray());
        REQUIRE(j.Size() == 3);
        REQUIRE(j[0] == "cpu");
        REQUIRE(j[1] == "gpu");
        REQUIRE(j[2] == "accelerator");

        f &= ~kl::make_flags(device_type::accelerator);

        j = kl::json::serialize(f);
        REQUIRE(j.IsArray());
        REQUIRE(j.Size() == 2);
        REQUIRE(j[0] == "cpu");
        REQUIRE(j[1] == "gpu");
    }

    SECTION("from json")
    {
        auto j = R"({"cpu": 1})"_json;
        REQUIRE_THROWS_WITH(kl::json::deserialize<device_flags>(j),
                            "type must be an array but is kObjectType");

        j = R"([])"_json;
        auto f = kl::json::deserialize<device_flags>(j);
        REQUIRE(f.underlying_value() == 0);

        j = R"(["cpu", "gpu"])"_json;
        f = kl::json::deserialize<device_flags>(j);

        REQUIRE(f.underlying_value() ==
                (kl::underlying_cast(device_type::cpu) |
                 kl::underlying_cast(device_type::gpu)));
    }
}

TEST_CASE("json dump")
{
    using namespace kl;

    SECTION("basic types")
    {
        CHECK(json::dump('a') == "97");
        CHECK(json::dump(1) == "1");
        CHECK(json::dump(3U) == "3");
        CHECK(json::dump(std::int64_t{-1}) == "-1");
        CHECK(json::dump(std::uint64_t{1}) == "1");
        CHECK(json::dump(true) == "true");
        CHECK(json::dump(nullptr) == "null");
        CHECK(json::dump("qwe") == "\"qwe\"");
        CHECK(json::dump(std::string{"qwe"}) == "\"qwe\"");
        CHECK(json::dump(13.11) == "13.11");
        CHECK(json::dump(ordinary_enum::oe_one) == "0");
    }

    SECTION("inner_t")
    {
        CHECK(json::dump(inner_t{}) == R"({"r":1337,"d":3.1459259999999999})");
    }

    SECTION("different types and 'modes' for enums")
    {
        CHECK(json::dump(enums{}) ==
              R"({"e0":0,"e1":0,"e2":"oe_one_ref","e3":"one"})");
    }

    SECTION("test unsigned types")
    {
        auto res = json::dump(unsigned_test{});
        CHECK(
            res ==
            R"({"u8":128,"u16":32768,"u32":4294967295,"u64":18446744073709551615})");
    }

    SECTION("enum_flags")
    {
        auto f = make_flags(device_type::cpu) | device_type::gpu |
                 device_type::accelerator;
        auto res = json::dump(f);
        CHECK(res == R"(["cpu","gpu","accelerator"])");

        f &= ~kl::make_flags(device_type::accelerator);
        res = json::dump(f);
        CHECK(res == R"(["cpu","gpu"])");
    }

    SECTION("tuple")
    {
        auto t = std::make_tuple(13, 3.14, colour_space::lab, true);
        auto res = json::dump(t);
        CHECK(res == R"([13,3.14,"lab",true])");
    }

    SECTION("skip serializing optional fields")
    {
        optional_test t;
        t.non_opt = 23;

        CHECK(json::dump(t) == R"({"non_opt":23})");

        t.opt = 78;
        CHECK(json::dump(t) == R"({"non_opt":23,"opt":78})");
    }

    SECTION("don't skip optional fields if requested")
    {
        optional_test t;
        t.non_opt = 23;

        using namespace rapidjson;

        StringBuffer sb;
        Writer<StringBuffer> writer{sb};
        kl::json::dump_context<Writer<StringBuffer>> ctx{writer, false};

        json::dump(t, ctx);
        std::string res = sb.GetString();
        CHECK(res == R"({"non_opt":23,"opt":null})");

        sb.Clear();
        writer.Reset(sb);

        t.opt = 78;
        json::dump(t, ctx);
        res = sb.GetString();
        CHECK(res == R"({"non_opt":23,"opt":78})");
    }

    SECTION("complex structure with std/boost containers")
    {
        auto res = json::dump(test_t{});
        CHECK(
            res ==
            R"({"hello":"world","t":true,"f":false,"i":123,)"
            R"("pi":3.1415998935699465,"a":[1,2,3,4],"ad":[[1,2],[3,4,5]],)"
            R"("space":"lab","tup":[1,3.140000104904175,"QWE"],"map":{"1":"hls","2":"rgb"},)"
            R"("inner":{"r":1337,"d":3.1459259999999999}})");
    }

    SECTION("unsigned: check value greater than 0x7FFFFFFU")
    {
        REQUIRE(json::dump(2147483648U) == "2147483648");
    }

    SECTION("std containers")
    {
        auto j1 = json::dump(std::vector<inner_t>{inner_t{}});
        CHECK(j1 == R"([{"r":1337,"d":3.1459259999999999}])");

        auto j2 = json::dump(std::list<inner_t>{inner_t{}});
        CHECK(j2 == R"([{"r":1337,"d":3.1459259999999999}])");

        auto j3 = json::dump(std::deque<inner_t>{inner_t{}});
        CHECK(j3 == R"([{"r":1337,"d":3.1459259999999999}])");

        auto j4 =
            json::dump(std::map<std::string, inner_t>{{"inner1", inner_t{}}});
        CHECK(j4 == R"({"inner1":{"r":1337,"d":3.1459259999999999}})");

        auto j5 = json::dump(
            std::unordered_map<std::string, inner_t>{{"inner2", inner_t{}}});
        CHECK(j5 == R"({"inner2":{"r":1337,"d":3.1459259999999999}})");
    }
}

namespace {

class my_dump_context
{
public:
    using writer_type = rapidjson::Writer<rapidjson::StringBuffer>;

    explicit my_dump_context(writer_type& writer) : writer_{writer} {}

    writer_type& writer() const { return writer_; }

    template <typename Key, typename Value>
    bool skip_field(const Key& key, const Value& value)
    {
        return !std::strcmp(key, "secret");
    }

private:
    writer_type& writer_;
};
} // namespace

TEST_CASE("json dump - custom context")
{
    using namespace kl;
    using namespace rapidjson;

    StringBuffer sb;
    Writer<StringBuffer> writer{sb};
    my_dump_context ctx{writer};

    json::dump(struct_with_blacklisted{}, ctx);
    std::string res = sb.GetString();
    CHECK(res == R"({"value":34,"other_non_secret":true})");
}

namespace kl {
namespace json {

template <>
struct encoder<std::chrono::seconds>
{
    template <typename Context>
    static void encode(const std::chrono::seconds& s, Context& ctx)
    {
        json::dump(s.count(), ctx);
    }
};
} // namespace json
} // namespace kl

TEST_CASE("json dump - extended")
{
    using namespace std::chrono;
    chrono_test t{2, seconds{10}, {seconds{6}, seconds{12}}};
    const auto res = kl::json::dump(t);
    CHECK(res == R"({"t":2,"sec":10,"secs":[6,12]})");
}

template <typename Context>
void encode(global_struct, Context& ctx)
{
    kl::json::dump("global_struct", ctx);
}

namespace my {

template <typename Context>
void encode(none_t, Context& ctx)
{
    kl::json::dump(nullptr, ctx);
}

template <typename T, typename Context>
void encode(const value_wrapper<T>& t, Context& ctx)
{
    kl::json::dump(t.value, ctx);
}
} // namespace my

TEST_CASE("json dump - overloading")
{
    aggregate a{{}, {}, {31}};
    auto res = kl::json::dump(a);
    CHECK(res == R"({"g":"global_struct","n":null,"w":31})");
}
