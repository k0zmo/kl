#include "kl/zip.hpp"

#include <catch/catch.hpp>

#include <vector>
#include <array>
#include <string>

TEST_CASE("zip")
{
    std::vector<int> v1 = {50, 40, 30, 20, 10};
    const float v2[] = {3.14f, 2.74f, 1.61f};
    const std::array<std::string, 3> v3 = {"ABC", "ZXC", "QWE"};

    SECTION("enumerate vector")
    {
        std::vector<std::size_t> indices;
        std::vector<int> values;

        kl::enumerate(v1, [&](std::size_t index, int v) {
            indices.push_back(index);
            values.push_back(v);
        });

        REQUIRE(indices.size() == v1.size());
        REQUIRE(values.size() == v1.size());

        for (std::size_t i = 0U; i < v1.size(); ++i)
        {
            REQUIRE(indices[i] == i);
            REQUIRE(values[i] == v1[i]);
        }
    }

    SECTION("size of zipped sequence")
    {
        // Size of zipped sequence is equal to the length of shortest one
        REQUIRE(kl::make_zip(v1, v2, v3).size() == 3);
        REQUIRE(kl::make_zip(v2, v3).size() == 3);
        REQUIRE(kl::make_zip(v1).size() == 5);
    }

    SECTION("zippeds sequence")
    {
        std::vector<int> z1;
        std::vector<float> z2;
        std::vector<std::string> z3;

        kl::zip_for_each(kl::make_zip(v1, v2, v3),
                         [&](int i, float j, const std::string& k) {
                             z1.push_back(i);
                             z2.push_back(j);
                             z3.push_back(k);
                         });

        REQUIRE(z1.size() == 3);
        REQUIRE(z2.size() == 3);
        REQUIRE(z3.size() == 3);

        for (std::size_t i = 0U; i < 3; ++i)
        {
            REQUIRE(z1[i] == v1[i]);
            REQUIRE(z2[i] == v2[i]);
            REQUIRE(z3[i] == v3[i]);
        }
    }

    SECTION("for range loop for zipped sequence")
    {
        std::vector<int> z1;
        std::vector<float> z2;
        std::vector<std::string> z3;

        for (const auto& z : kl::make_zip(v1, v2, v3))
        {
            static_assert(std::is_same<decltype(std::get<0>(z)),
                int&>::value, "???");
            static_assert(std::is_same<decltype(std::get<1>(z)),
                const float&>::value, "???");
            static_assert(std::is_same<decltype(std::get<2>(z)),
                const std::string&>::value, "???");

            z1.push_back(std::get<0>(z));
            z2.push_back(std::get<1>(z));
            z3.push_back(std::get<2>(z));
        }

        REQUIRE(z1.size() == 3);
        REQUIRE(z2.size() == 3);
        REQUIRE(z3.size() == 3);

        for (std::size_t i = 0U; i < 3; ++i)
        {
            REQUIRE(z1[i] == v1[i]);
            REQUIRE(z2[i] == v2[i]);
            REQUIRE(z3[i] == v3[i]);
        }
    }

    SECTION("integral range")
    {
        std::vector<std::size_t> indices;

        for (auto i : kl::make_integral_range(v1))
        {
            static_assert(
                std::is_same<decltype(i), decltype(v1)::size_type>::value,
                "???");
            indices.push_back(i);
        }

        REQUIRE(v1.size() == indices.size());
        REQUIRE(indices[0] == 0);
        REQUIRE(indices[1] == 1);
        REQUIRE(indices[2] == 2);
        REQUIRE(indices[3] == 3);
        REQUIRE(indices[4] == 4);
    }
}
