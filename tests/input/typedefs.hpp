#pragma once

#include "kl/enum_set.hpp"
#include "kl/reflect_enum.hpp"
#include "kl/reflect_struct.hpp"

#include <vector>
#include <map>
#include <string>
#include <optional>
#include <chrono>

struct optional_test
{
    int non_opt;
    std::optional<int> opt;
};
KL_REFLECT_STRUCT(optional_test, non_opt, opt)

struct inner_t
{
    int r = 1337;
    double d = 3.145926;
};
KL_REFLECT_STRUCT(inner_t, r, d)

struct Manual
{
    inner_t a;
    int b = 416;
    double c = 2.71828;
};
template <typename Visitor, typename Self>
constexpr void reflect_struct(Visitor&& vis, Self&& self,
                              ::kl::record_class<Manual>)
{
    vis(self.a.r, "Ar");
    vis(self.a.d, "Ad");
    vis(self.b, "B");
    vis(self.c, "C");
}
constexpr std::size_t reflect_num_fields(::kl::record_class<Manual>) noexcept
{
    return 4U;
}

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
KL_REFLECT_ENUM(colour_space, rgb, xyz, ycrcb, hsv, lab, hls, luv)

enum class device_type
{
    default_ = (1 << 0),
    cpu = (1 << 1),
    gpu = (1 << 2),
    accelerator = (1 << 3),
    custom = (1 << 4)
};
KL_REFLECT_ENUM(device_type,
                 (default_, default), cpu, gpu, accelerator, custom)
using device_flags = kl::enum_set<device_type>;

// on GCC underlying_type(ordinary_enum) => unsigned
enum ordinary_enum : int { oe_one };
enum class scope_enum { one };
enum ordinary_enum_reflectable { oe_one_ref };
enum class scope_enum_reflectable { one };

KL_REFLECT_ENUM(ordinary_enum_reflectable, oe_one_ref)
KL_REFLECT_ENUM(scope_enum_reflectable, one)

struct enums
{
    ordinary_enum e0 = ordinary_enum::oe_one;
    scope_enum e1 = scope_enum::one;
    ordinary_enum_reflectable e2 = ordinary_enum_reflectable::oe_one_ref;
    scope_enum_reflectable e3 = scope_enum_reflectable::one;
};
KL_REFLECT_STRUCT(enums, e0, e1, e2, e3)

struct test_t
{
    std::string hello = "world";
    bool t = true;
    bool f = false;
    std::optional<int> n;
    int i = 123;
    float pi = 3.1416f;
    std::vector<int> a = {1, 2, 3, 4};
    std::vector<std::vector<int>> ad = {std::vector<int>{1, 2},
                                        std::vector<int>{3, 4, 5}};
    colour_space space = colour_space::lab;
    std::tuple<int, double, std::string> tup = std::make_tuple(1, 3.14f, "QWE");

    std::map<std::string, colour_space> map = {{"1", colour_space::hls},
                                               {"2", colour_space::rgb}};

    inner_t inner;
};
KL_REFLECT_STRUCT(test_t, hello, t, f, n, i, pi, a, ad, space, tup, map, inner)

struct unsigned_test
{
    std::uint8_t u8{128};
    std::uint16_t u16{32768};
    std::uint32_t u32{(std::numeric_limits<std::uint32_t>::max)()};
    std::uint64_t u64{(std::numeric_limits<std::uint64_t>::max)()};
};
KL_REFLECT_STRUCT(unsigned_test, u8, u16, u32, u64)

struct signed_test
{
    std::int8_t i8{-127};
    std::int16_t i16{-13768};
    std::int32_t i32{(std::numeric_limits<std::int32_t>::min)()};
    std::int64_t i64{(std::numeric_limits<std::int64_t>::min)()};
};
KL_REFLECT_STRUCT(signed_test, i8, i16, i32, i64)

struct chrono_test
{
    int t;
    std::chrono::seconds sec;
    std::vector<std::chrono::seconds> secs;
};
KL_REFLECT_STRUCT(chrono_test, t, sec, secs)

struct global_struct {};

namespace my {

struct none_t {};

template <typename T>
struct value_wrapper
{
    T value;
};
}

struct aggregate
{
    global_struct g;
    my::none_t n;
    my::value_wrapper<int> w;
};
KL_REFLECT_STRUCT(aggregate, g, n, w)

struct struct_with_blacklisted
{
    int value{34};
    float secret{3.2f};
    bool other_non_secret{true};
};
KL_REFLECT_STRUCT(struct_with_blacklisted, value, secret, other_non_secret)
