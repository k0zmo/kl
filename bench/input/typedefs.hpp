#pragma once

#include "kl/reflect_enum.hpp"
#include "kl/reflect_struct.hpp"

#include <vector>
#include <map>
#include <string>
#include <optional>

struct inner_t
{
    int r = 1337;
    double d = 3.145926;
};
KL_REFLECT_STRUCT(inner_t, r, d)

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
