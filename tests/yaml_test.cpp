#include "kl/yaml.hpp"
#include "kl/ctti.hpp"
#include "kl/enum_flags.hpp"
#include "input/typedefs.hpp"

#include <catch2/catch.hpp>
#include <boost/optional/optional_io.hpp>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <tuple>
#include <deque>
#include <list>
#include <map>

TEST_CASE("yaml")
{
    using namespace kl;

    SECTION("basic types")
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

        const char* qwe = "qwe";
        CHECK(yaml::serialize(qwe).as<std::string>() == "qwe");
    }

    SECTION("parse error")
    {
        REQUIRE_NOTHROW(R"([])"_yaml);
        REQUIRE_THROWS_AS(R"([{]})"_yaml, yaml::parse_error);
        REQUIRE_THROWS_WITH(
            R"([{]})"_yaml,
            "yaml-cpp: error at line 1, column 3: illegal flow end");
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

    SECTION("serialize different types and 'modes' for enums")
    {
        auto y = yaml::serialize(enums{});
        REQUIRE(y["e0"].as<int>() == 0);
        REQUIRE(y["e1"].as<int>() == 0);
        REQUIRE(y["e2"].as<std::string>() == "oe_one_ref");
        REQUIRE(y["e3"].as<std::string>() == "one");
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

        YAML::Node doc;
        yaml::serialize_context ctx{doc, false};

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

    SECTION("test unsigned types")
    {
        unsigned_test t;
        auto y = yaml::serialize(t);

        REQUIRE(y["u8"].as<unsigned char>() == t.u8);
        REQUIRE(y["u16"].as<unsigned short>() == t.u16);
        REQUIRE(y["u32"].as<unsigned int>() == t.u32);
        REQUIRE(y["u64"].as<std::uint64_t>() == t.u64);
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
}

TEST_CASE("yaml - enum_flags")
{
    SECTION("to yaml")
    {
        auto f = kl::make_flags(device_type::cpu) | device_type::gpu |
                 device_type::accelerator;
        auto y = kl::yaml::serialize(f);
        REQUIRE(y.IsSequence());
        REQUIRE(y.size() == 3);
        REQUIRE(y[0].as<std::string>() == "cpu");
        REQUIRE(y[1].as<std::string>() == "gpu");
        REQUIRE(y[2].as<std::string>() == "accelerator");

        f &= ~kl::make_flags(device_type::accelerator);

        y = kl::yaml::serialize(f);
        REQUIRE(y.IsSequence());
        REQUIRE(y.size() == 2);
        REQUIRE(y[0].as<std::string>() == "cpu");
        REQUIRE(y[1].as<std::string>() == "gpu");
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
        CHECK(yaml::dump(enums{}) ==
              "e0: 0\ne1: 0\ne2: oe_one_ref\ne3: one");
    }

    SECTION("test unsigned types")
    {
        auto res = yaml::dump(unsigned_test{});
        CHECK(
            res ==
            "u8: \"\\x80\"\nu16: 32768\nu32: 4294967295\nu64: 18446744073709551615");
    }

    SECTION("enum_flags")
    {
        auto f = make_flags(device_type::cpu) | device_type::gpu |
                 device_type::accelerator;
        auto res = yaml::dump(f);
        CHECK(res == "- cpu\n- gpu\n- accelerator");

        f &= ~kl::make_flags(device_type::accelerator);
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
        CHECK(
            res ==
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
    explicit my_dump_context(YAML::Emitter& emitter)
        : emitter_{emitter}
    {
    }

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
