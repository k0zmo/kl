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

    SECTION("parse error")
    {
        REQUIRE_NOTHROW(R"([])"_yaml);
        REQUIRE_THROWS_AS(R"([{]})"_yaml, yaml::parse_error);
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
        auto j1 = yaml::dump(std::vector<inner_t>{inner_t{}});
        CHECK(j1 == "- r: 1337\n  d: 3.145926");

        auto j2 = yaml::dump(std::list<inner_t>{inner_t{}});
        CHECK(j2 == "- r: 1337\n  d: 3.145926");

        auto j3 = yaml::dump(std::deque<inner_t>{inner_t{}});
        CHECK(j3 == "- r: 1337\n  d: 3.145926");

        auto j4 =
            yaml::dump(std::map<std::string, inner_t>{{"inner1", inner_t{}}});
        CHECK(j4 == "inner1:\n  r: 1337\n  d: 3.145926");

        auto j5 = yaml::dump(
            std::unordered_map<std::string, inner_t>{{"inner2", inner_t{}}});
        CHECK(j5 == "inner2:\n  r: 1337\n  d: 3.145926");
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
