#include "kl/ctti.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/stream_join.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <ios>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
    KL_REFLECT_STRUCT_DERIVED_FRIEND(T, (A, S), d, e)

    std::string d;
    std::vector<std::string> e;
};
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
template <typename Factory, typename Visitor>
[[maybe_unused]] constexpr void reflect_struct_fields(Factory&& factory, Visitor&& vis,
                                                      ::kl::ctti::record_class<ZZZ>)
{
    vis(KL_ACCESSOR_FIELD(a));
    vis(KL_ACCESSOR_FIELD(b));
    vis(KL_ACCESSOR_FIELD(ref_a));
    vis(KL_ACCESSOR_FIELD(ref_b));
}

[[maybe_unused]] constexpr std::size_t
    reflect_num_fields(::kl::ctti::record_class<ZZZ>) noexcept
{
    return 4;
}

struct attr_base {};
struct attr_derived : attr_base {};
struct attr_value
{
    int value;
};

struct WithAttrs
{
    int visible;
    int annotated;
};
KL_REFLECT_STRUCT(WithAttrs, visible, (annotated, attr_derived{}, attr_value{42}))

struct has_attr_tester
{
    int a;
    int b;
    int c;
    KL_REFLECT_STRUCT_FRIEND(has_attr_tester, (a, attr_base{}), (b), (c, attr_value{3}))
};
static_assert(kl::ctti::has_any_attribute<has_attr_tester, attr_base>());
static_assert(!kl::ctti::has_any_attribute<has_attr_tester, attr_derived>());
static_assert(kl::ctti::has_any_attribute<has_attr_tester, attr_value>());
static_assert(kl::ctti::has_any_attribute<has_attr_tester, attr_base, attr_value>());
static_assert(kl::ctti::has_any_attribute<has_attr_tester, attr_derived, attr_base>());

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

std::ostream& operator<<(std::ostream& os, const A& a)
{
    kl::ctti::reflect_object(a, [&os](auto field) {
        os << field.name() << ": " << field.value() << "\n";
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
        REQUIRE(!kl::ctti::is_reflectable_v<T>);
    }

    SECTION("global type A")
    {
        REQUIRE(kl::ctti::is_reflectable_v<A>);
        REQUIRE(kl::ctti::num_fields<A>() == 2);

        std::ostringstream ss;
        A a = {"ZXC", {1,2,3}, 0};
        kl::ctti::reflect_object(a, [&ss](auto field){
            ss << field.name() << ": " << field.value() << "\n";
        });

        REQUIRE(ss.str() == "x: ZXC\ny: 1, 2, 3\n");
    }

    SECTION("field attributes")
    {
        WithAttrs value{1, 2};
        unsigned acc{};
        kl::ctti::reflect_object(value, [&](auto field) {
            if (acc == 0)
            {
                CHECK(field.name() == "visible"s);
                CHECK(field.value() == 1);
                CHECK(!field.template has<attr_base>());
            }
            else
            {
                CHECK(field.name() == "annotated"s);
                CHECK(field.value() == 2);
                CHECK(field.template has<attr_base>());
                REQUIRE(field.template get<attr_base>() != nullptr);
                REQUIRE(field.template get<attr_value>() != nullptr);
                CHECK(field.template get<attr_value>()->value == 42);
            }
            ++acc;
        });

        CHECK(acc == 2);
    }

    SECTION("global type B, derives from A")
    {
        using B = ns::B;

        REQUIRE(kl::ctti::is_reflectable_v<B>);
        REQUIRE(kl::ctti::num_fields<B>() == 3);

        std::ostringstream ss;
        B b;
        b.x = "QWE";
        b.y = {0, 1337};
        b.zzz = 123;
        b.z = 0;
        kl::ctti::reflect_object(b, [&ss](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
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
        kl::ctti::reflect_object(ref_b, [&ss](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
        });

        REQUIRE(ss.str() == "x: QWE\ny: 0, 1337\nzzz: 123\n");
    }

    SECTION("type S in namespace ns with std::array<>")
    {
        REQUIRE(kl::ctti::is_reflectable_v<S>);
        static_assert(kl::ctti::num_fields<S>() == 4);

        const S s = {5, false, {3.14f}, A{"ZXC", {1, 2, 3, 4, 5, 6}, 0}};

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect_object(s, [&ss](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
        });

        REQUIRE(ss.str() == "a: 5\nb: false\nc: 3.14, 0, 0\naa: x: ZXC\ny: 1, "
                            "2, 3, 4, 5, 6\n\n");
    }

    SECTION("type Sdev")
    {
        REQUIRE(kl::ctti::is_reflectable_v<Sdev>);
        static_assert(kl::ctti::num_fields<Sdev>() == 3);

        Sdev sd{};
        sd.a = 3400;
        sd.b = true;
        sd.adev = 114;

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect_object(sd, [&ss](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
        });

        REQUIRE(ss.str() == "a: 3400\nb: true\nadev: 114\n");
    }

    SECTION("type T with multi-inheritance in 2-level deep namespace")
    {
        using T = ns::inner::T;

        REQUIRE(kl::ctti::is_reflectable_v<T>);
        REQUIRE(kl::ctti::num_fields<T>() == 2 + 2+ 4);

        T t{"HELLO",{"WORLD", "Hello"}};
        t.a = 2;
        t.b = true;
        t.c = {{2.71f, 3.14f, 1.67f}};

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect_object(t, [&ss](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
        });
        REQUIRE(ss.str() == "x: \ny: .\na: 2\nb: true\nc: 2.71, 3.14, "
                            "1.67\naa: x: \ny: .\n\nd: HELLO\ne: WORLD, "
                            "Hello\n");
    }

    SECTION("type ZZZ, with references")
    {
        REQUIRE(kl::ctti::is_reflectable_v<ZZZ>);
        static_assert(kl::ctti::num_fields<ZZZ>() == 4);

        const ZZZ zzz = {5, 11};

        std::ostringstream ss;
        ss << std::boolalpha;
        kl::ctti::reflect_object(zzz, [&ss](auto field) {
            ss << field.name() << ": " << field.value() << "\n";
        });

        REQUIRE(ss.str() == "a: 5\nb: 11\nref_a: 5\nref_b: 11\n");
    }
}
