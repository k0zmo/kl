#include "kl/reflect_struct.hpp"

#include <catch2/catch.hpp>

#include <string>

namespace {

struct A
{
    int i;
    bool b;
    double d;
};
KL_REFLECT_STRUCT(A, i, b, d)

struct B : A
{
    unsigned long long ull;
};
KL_REFLECT_STRUCT_DERIVED(B, A, ull)

} // namespace

TEST_CASE("reflect record")
{
    using namespace std::string_literals;

    A a{2, true, 3.14};
    std::tuple<int, bool, double> t{};
    unsigned acc{};
    reflect_struct(
        [&](auto& field, auto name) {
            switch (acc)
            {
            case 0:
                CHECK(name == "i"s);
                std::get<0>(t) = a.i;
                break;
            case 1:
                CHECK(name == "b"s);
                std::get<1>(t) = a.b;
                break;
            case 2:
                CHECK(name == "d"s);
                std::get<2>(t) = a.d;
                break;
            }
            ++acc;
        },
        a, kl::record<A>);

    CHECK(std::get<0>(t) == a.i);
    CHECK(std::get<1>(t) == a.b);
    CHECK(std::get<2>(t) == a.d);

    const B b = [&]() {
        B b;
        b.ull = 3112;
        static_cast<A&>(b) = a;
        return b;
    }();

    std::string last_name;
    acc = {};
    reflect_struct(
        [&](auto& field, auto name) {
            ++acc;
            last_name = name;
        },
        b, kl::record<B>);
    CHECK(acc == 4);
    CHECK(last_name == "ull");
}
