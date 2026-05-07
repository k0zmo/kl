#include "kl/reflect_struct.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
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

TEST_CASE("reflect struct")
{
    using namespace std::string_literals;

    A a{2, true, 3.14};
    std::ostringstream ss;
    reflect_struct(
        [&](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
        },
        a, kl::ctti::record<A>);

    CHECK(ss.str() == "i: 2\nb: 1\nd: 3.14\n");

    const B b = [&]() {
        B b;
        b.ull = 3112;
        static_cast<A&>(b) = a;
        return b;
    }();

    std::string last_name;
    unsigned acc{};
    reflect_struct(
        [&](auto field) {
            ++acc;
            last_name = field.name();
        },
        b, kl::ctti::record<B>);
    CHECK(acc == 4);
    CHECK(last_name == "ull");
}
