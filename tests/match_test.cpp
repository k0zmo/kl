#include "kl/match.hpp"

#include <catch2/catch_test_macros.hpp>

#include <variant>

namespace test {

bool foo(int) { return true; }
bool bar(double) { return true; }

struct dummy {};
} // namespace test

TEST_CASE("match")
{
    SECTION("overloader")
    {
        auto f = kl::overloader{[](int i) { return i * 2.0f; },
                                [](float f) { return f * 2.5f; }};
        REQUIRE(f(2) == 4.0f);
        REQUIRE(f(2.0f) == 5.0f);
    }

    SECTION("match")
    {
        std::variant<int, float, bool> v = true;

        auto res = kl::match(v, [](int i) { return i * 2.0f; },
                                [](float f) { return f * 2.5f; },
                                [](bool b) { return b * 2.0f; });
        REQUIRE(res == 2.0f);
    }

    SECTION("match func")
    {
        std::variant<int, double, test::dummy> v = test::dummy{};

        auto res = kl::match(v, kl::func_overload{&test::foo},
                                kl::func_overload{&test::bar},
                                [](const test::dummy&) { return false; });
        REQUIRE(!res);

        v = 3;
        res = kl::match(v, kl::func_overload{&test::foo},
                           kl::func_overload{&test::bar},
                           [](const test::dummy&) { return false; });
        REQUIRE(res);
    }
}
