#include "kl/type_class.hpp"

#include <catch2/catch.hpp>

namespace {

struct X {};
}

TEST_CASE("type_class")
{
    using namespace kl::type_class;

    static_assert(equals<X, unknown>::value, "!!!");
    static_assert(equals<decltype(3u), integral>::value, "!!!");
    static_assert(equals<std::map<std::string, int>, map>::value, "!!!");
}
