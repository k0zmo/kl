#include "kl/ctti.hpp"
#include "kl/stream_join.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <array>
#include <sstream>

namespace test {

struct A
{
    std::string x;
    std::vector<int> y;
    int z; // Not reflectable
};
KL_REFLECT_STRUCT(A, x, y)

struct S
{
    int a;
    bool b;
    std::array<float, 3> c;
    A aa;
};
KL_REFLECT_STRUCT(S, a, b, c, aa)

struct Sdev : S
{
    int adev;
};
KL_REFLECT_STRUCT(Sdev, a, b, adev)

namespace ns {
namespace inner {

class T : public A, public S
{
public:
    explicit T(std::string d, std::vector<std::string> e)
        : d(std::move(d)), e(std::move(e))
    {
    }

private:
    // because `d` and `e` are private
    template <typename Visitor, typename Self>
    friend constexpr void reflect_struct(Visitor&&, Self&&,
                                         ::kl::record_class<T>);
    std::string d;
    std::vector<std::string> e;
};
KL_REFLECT_STRUCT_DERIVED(T, (A, S), d, e)
} // namespace inner

struct B : A
{
    int zzz;
};
KL_REFLECT_STRUCT_DERIVED(B, A, zzz)
} // namespace ns

struct ZZZ
{
    int a;
    const int b;
    int& ref_a = a;
    const int& ref_b = b;
};
KL_REFLECT_STRUCT(ZZZ, a, b, ref_a, ref_b)
} // namespace test

namespace {

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    return os << kl::stream_join(v);
}

template <typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& v)
{
    return os << kl::stream_join(v);
}

template <typename T>
std::ostream& trampoline_operator(std::ostream& os, const T& obj)
{
    os << obj;
    return os;
}
} // namespace

namespace test {

std::ostream& operator<<(std::ostream& os, const A& a)
{
    kl::ctti::reflect(a, [&os](auto& field, auto name) {
        os << name << ": ";
        // This is used so GCC 7 and 8 will also consider `operator<<` for
        // vector<T> and array<T,N> defined above (see
        // https://godbolt.org/z/dw9DdN)
        trampoline_operator(os, field);
        os << "\n";
    });
    return os;
}
} // namespace test

TEST_CASE("ctti")
{
    using namespace std::string_literals;
    using namespace test;

    SECTION("name of simplest types")
    {
        CHECK(kl::ctti::name<int>() == "int");
    }

    SECTION("type not registered")
    {
        using T = std::vector<int>;
        REQUIRE(!kl::ctti::is_reflectable<T>);
    }

    SECTION("global type A")
    {
        REQUIRE(kl::ctti::is_reflectable<A>);
        REQUIRE(kl::ctti::num_fields<A>() == 2);

        std::ostringstream ss;
        A a = {"ZXC", {1,2,3}, 0};
        kl::ctti::reflect(a, [&ss](auto& field, auto name){
            ss << name << ": " << field << "\n";
        });

        REQUIRE(ss.str() == "x: ZXC\ny: 1, 2, 3\n");
    }

    SECTION("global type B, derives from A")
    {
        using B = ns::B;

        REQUIRE(kl::ctti::is_reflectable<B>);
        REQUIRE(kl::ctti::num_fields<B>() == 3);

        std::ostringstream ss;
        B b;
        b.x = "QWE";
        b.y = {0, 1337};
        b.zzz = 123;
        b.z = 0;
        kl::ctti::reflect(b, [&ss](auto& field, auto name) {
            ss << name << ": " << field << "\n";
        });

        REQUIRE(ss.str() == "x: QWE\ny: 0, 1337\nzzz: 123\n");

    }

    SECTION("const value of type B which derives from A")
    {
        using B = ns::B;
        B b;
        b.x = "QWE";
        b.y = {0, 1337};
        b.zzz = 123;
        b.z = 0;
        const B& ref_b = b;

        std::ostringstream ss;
        kl::ctti::reflect(ref_b, [&ss](auto& field, auto name) {
            ss << name << ": " << field << "\n";
        });

        REQUIRE(ss.str() == "x: QWE\ny: 0, 1337\nzzz: 123\n");
    }

    SECTION("type S in namespace ns with std::array<>")
    {
        REQUIRE(kl::ctti::is_reflectable<S>);
        static_assert(kl::ctti::num_fields<S>() == 4);

        const S s = {5, false, {3.14f}, A{"ZXC", {1, 2, 3, 4, 5, 6}, 0}};

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect(s, [&ss](auto& field, auto name) {
            ss << name << ": " << field << "\n";
        });

        REQUIRE(ss.str() == "a: 5\nb: false\nc: 3.14, 0, 0\naa: x: ZXC\ny: 1, "
                            "2, 3, 4, 5, 6\n\n");
    }

    SECTION("type Sdev")
    {
        REQUIRE(kl::ctti::is_reflectable<Sdev>);
        static_assert(kl::ctti::num_fields<Sdev>() == 3);

        Sdev sd{};
        sd.a = 3400;
        sd.b = true;
        sd.adev = 114;

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect(sd, [&ss](auto& field, auto name) {
            ss << name << ": " << field << "\n";
        });

        REQUIRE(ss.str() == "a: 3400\nb: true\nadev: 114\n");
    }

    SECTION("type T with multi-inheritance in 2-level deep namespace")
    {
        using T = ns::inner::T;

        REQUIRE(kl::ctti::is_reflectable<T>);
        REQUIRE(kl::ctti::num_fields<T>() == 2 + 2+ 4);

        T t{"HELLO",{"WORLD", "Hello"}};
        t.a = 2;
        t.b = true;
        t.c = {{2.71f, 3.14f, 1.67f}};

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect(t, [&ss](auto& field, auto name) {
            ss << name << ": " << field << "\n";
        });
        REQUIRE(ss.str() == "x: \ny: .\na: 2\nb: true\nc: 2.71, 3.14, "
                            "1.67\naa: x: \ny: .\n\nd: HELLO\ne: WORLD, "
                            "Hello\n");
    }
}
