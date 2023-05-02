#include "kl/reflect_enum.hpp"

#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <string>

namespace {

enum class A
{
    a,
    b,
    c
};

KL_REFLECT_ENUM(A, a, (b, BB), c)

} // namespace

TEST_CASE("reflect enum")
{
    using namespace std::string_literals;
    const auto rng = reflect_enum(kl::enum_<A>);

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

    REQUIRE_FALSE(it != rng.end());
}

namespace {

// clang-format off
enum class abcd
{
    a000, a001, a002, a003, a004, a005, a006, a007, a008, a009,
    a010, a011, a012, a013, a014, a015, a016, a017, a018, a019,
    a020, a021, a022, a023, a024, a025, a026, a027, a028, a029,
    a030, a031, a032, a033, a034, a035, a036, a037, a038, a039,
    a040, a041, a042, a043, a044, a045, a046, a047, a048, a049,
    a050, a051, a052, a053, a054, a055, a056, a057, a058, a059,
    a060, a061, a062, a063, a064, a065, a066, a067, a068, a069,
    a070, a071, a072, a073, a074, a075, a076, a077, a078, a079
};
KL_REFLECT_ENUM(abcd,
    a000,a001,a002,a003,a004,a005,a006,a007,a008,a009,
    a010,a011,a012,a013,a014,a015,a016,a017,a018,a019,
    a020,a021,a022,a023,a024,a025,a026,a027,a028,a029,
    a030,a031,a032,a033,a034,a035,a036,a037,a038,a039,
    a040,a041,a042,a043,a044,a045,a046,a047,a048,a049,
    a050,a051,a052,a053,a054,a055,a056,a057,a058,a059,
    a060,a061,a062,a063,a064,a065,a066,a067,a068,a069,
    a070,a071,a072,a073,a074,a075,a076,a077,a078,a079
)
// clang-format on
}

TEST_CASE("reflect big enum")
{
    using namespace std::string_literals;
    const auto rng = reflect_enum(kl::enum_<abcd>);

    REQUIRE(rng.size() == 80);

    auto it = rng.begin();
    REQUIRE(it != rng.end());
    CHECK(it->name == "a000"s);
    CHECK(it->value == abcd::a000);

    std::advance(it, 41);

    REQUIRE(it != rng.end());
    CHECK(it->name == "a041"s);
    CHECK(it->value == abcd::a041);

    std::advance(it, 38);
    REQUIRE(it != rng.end());
    CHECK(it->name == "a079"s);
    CHECK(it->value == abcd::a079);

    ++it;
    REQUIRE_FALSE(it != rng.end());
}
