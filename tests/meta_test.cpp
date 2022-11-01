#include "kl/meta.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("meta")
{
    SECTION("filter optionals")
    {
        static_assert(
            std::is_same<
                kl::type_pack<void*, int*>,
                kl::filter_t<std::is_pointer, void*, bool&, int*>>::value);

        using input = kl::type_pack<int, void, std::string, float>;
        using inter = kl::filter_t<std::is_arithmetic, input>;
        using output = kl::filter_t<std::is_integral, inter>;

        static_assert(std::is_same<kl::type_pack<int, float>, inter>::value);
        static_assert(std::is_same<kl::type_pack<int>, output>::value);
    }
}
