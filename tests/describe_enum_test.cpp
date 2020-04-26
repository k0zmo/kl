#include "kl/describe_enum.hpp"

#include <catch2/catch.hpp>

#include <string>

namespace {

enum class A
{
    a,
    b,
    c
};

KL_DESCRIBE_ENUM(A, a, (b, BB), c)
} // namespace

TEST_CASE("describe enum")
{
    using namespace std::string_literals;
    const auto rng = describe_enum(kl::enum_<A>);

    REQUIRE(rng.size() == 3);
    auto it = rng.begin();
    REQUIRE(it != rng.end());
    CHECK(it->name == "a"s);
    CHECK(it->value == A::a);
    ++it;

    REQUIRE(it != rng.end());
    CHECK(it->name == "BB"s);
    CHECK(it->value == A::b);
    ++it;

    REQUIRE(it != rng.end());
    CHECK(it->name == "c"s);
    CHECK(it->value == A::c);
    ++it;

    REQUIRE(it == rng.end());
}
