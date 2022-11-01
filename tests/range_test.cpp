#include "kl/range.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <type_traits>

namespace {

struct fake_vec
{
    struct iterator
    {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = int*;
        using reference = int&;
    };

    iterator begin() noexcept(false) { return {}; }
    iterator end() noexcept(false) { return {}; }
};
} // namespace

TEST_CASE("range")
{
    SECTION("from vector")
    {
        std::vector<int> vec = {1, 2, 3};
        auto rng = kl::range(vec);
        REQUIRE(rng.size() == 3);
        static_assert(std::is_same<decltype(rng)::value_type, int>::value, "");
        static_assert(std::is_same<decltype(rng)::reference, int&>::value, "");
        static_assert(noexcept(kl::range(vec)), "");
        static_assert(noexcept(rng.begin()), "");

        fake_vec v2;
        static_assert(!noexcept(kl::range(v2)), "");
        auto rng2 = kl::range(v2);
        static_assert(noexcept(rng2.begin()), "");

        auto crng = kl::range(static_cast<const std::vector<int>&>(vec));
        REQUIRE(rng.size() == 3);
        static_assert(std::is_same<decltype(crng)::value_type, int>::value, "");
        static_assert(std::is_same<decltype(crng)::reference, const int&>::value, "");

        for (auto& i : rng) { (void)i; }
        for (auto& i : crng) { (void)i; }
    }

    SECTION("from iterator pair")
    {
        std::vector<int> vec = {1, 2, 3};
        auto rng = kl::range(vec.begin() + 1, vec.end());
        static_assert(std::is_same<decltype(rng)::value_type, int>::value, "");
        REQUIRE(rng.size() == 2);
        for (auto& i : rng) { (void)i; }
    }

    SECTION("from raw array")
    {
        bool arr[4]{};
        auto rng = kl::range(arr);
        REQUIRE(rng.size() == 4);
        for (auto& i : rng) { (void)i; }

        static_assert(std::is_same<decltype(rng)::value_type, bool>::value, "");
        static_assert(std::is_same<decltype(rng)::reference, bool&>::value, "");
        static_assert(noexcept(kl::range(arr)), "");

        const bool carr[4]{};
        auto crng = kl::range(carr);
        REQUIRE(crng.size() == 4);

        static_assert(std::is_same<decltype(crng)::value_type, bool>::value, "");
        static_assert(std::is_same<decltype(crng)::reference, const bool&>::value, "");

        for (auto& i : rng) { (void)i; }
        for (auto& i : crng) { (void)i; }
    }
}
