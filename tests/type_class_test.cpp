#include "kl/type_class.hpp"

#include <catch/catch.hpp>

namespace {

struct X {};
}

TEST_CASE("type_class")
{
    static_assert(
        std::is_same<kl::type_class::unknown, kl::type_class::get<X>>::value,
        "!!!");
    static_assert(std::is_same<kl::type_class::integral,
                               kl::type_class::get<decltype(3u)>>::value,
                  "!!!");

    static_assert(
        std::is_same<kl::type_class::map,
                     kl::type_class::get<std::map<std::string, int>>>::value,
        "!!!");
}
