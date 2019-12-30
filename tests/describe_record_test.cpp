#include "kl/describe_record.hpp"

#include <catch2/catch.hpp>

#include <string>

namespace {

struct A
{
    int i;
    bool b;
    double d;
};
KL_DESCRIBE_FIELDS(A, (i, b, d))

struct B : A
{
    unsigned long long ull;
};
KL_DESCRIBE_BASES(B, (A))
KL_DESCRIBE_FIELDS(B, (ull))
}

TEST_CASE("describe record")
{
    using namespace std::string_literals;

    static_assert(
        std::is_same<decltype(describe_bases(static_cast<B*>(nullptr))),
                     kl::type_pack<A>>::value,
        "");

    A a{2, true, 3.14};
    auto f1 = describe_fields(static_cast<A*>(nullptr), a);
    static_assert(std::tuple_size<decltype(f1)>::value == 3, "");
    CHECK(std::get<0>(f1).name() == "i"s);
    CHECK(std::get<0>(f1).get() == a.i);
    CHECK(std::get<1>(f1).name() == "b"s);
    CHECK(std::get<1>(f1).get() == a.b);
    CHECK(std::get<2>(f1).name() == "d"s);
    CHECK(std::get<2>(f1).get() == a.d);

    const B b = [&]() {
        B b;
        b.ull = 3112;
        static_cast<A&>(b) = a;
        return b;
    }();

    auto f2 = describe_fields(static_cast<B*>(nullptr), b);
    static_assert(std::tuple_size<decltype(f2)>::value == 1, "");
    CHECK(std::get<0>(f2).name() == "ull"s);
    CHECK(std::get<0>(f2).get() == b.ull);
}
