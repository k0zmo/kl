#include "kl/base_range.hpp"

#include <catch2/catch.hpp>
#include <vector>

TEST_CASE("base_range")
{
    SECTION("from vector")
    {
        std::vector<int> vec = {1, 2, 3};
        auto rng = kl::make_range(vec);
        REQUIRE(rng.size() == 3);
        static_assert(std::is_same<decltype(rng)::value_type, int>::value, "");
        static_assert(std::is_same<decltype(rng)::reference, int&>::value, "");

        auto crng = kl::make_range(static_cast<const std::vector<int>&>(vec));
        REQUIRE(rng.size() == 3);
        static_assert(std::is_same<decltype(crng)::value_type, int>::value, "");
        static_assert(std::is_same<decltype(crng)::reference, const int&>::value, "");

        for (auto& i : rng) { (void)i; }
        for (auto& i : crng) { (void)i; }
    }

    SECTION("from iterator pair")
    {
        std::vector<int> vec = {1, 2, 3};
        auto rng = kl::make_range(vec.begin() + 1, vec.end());
        static_assert(std::is_same<decltype(rng)::value_type, int>::value, "");
        REQUIRE(rng.size() == 2);
        for (auto& i : rng) { (void)i; }
    }

    SECTION("from raw array")
    {
        bool arr[4]{};
        auto rng = kl::make_range(arr);
        REQUIRE(rng.size() == 4);
        for (auto& i : rng) { (void)i; }

        static_assert(std::is_same<decltype(rng)::value_type, bool>::value, "");
        static_assert(std::is_same<decltype(rng)::reference, bool&>::value, "");

        const bool carr[4]{};
        auto crng = kl::make_range(carr);
        REQUIRE(crng.size() == 4);

        static_assert(std::is_same<decltype(crng)::value_type, bool>::value, "");
        static_assert(std::is_same<decltype(crng)::reference, const bool&>::value, "");

        for (auto& i : rng) { (void)i; }
        for (auto& i : crng) { (void)i; }
    }
}
