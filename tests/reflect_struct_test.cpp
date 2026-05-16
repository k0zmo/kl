#include "kl/reflect_struct.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <utility>

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

struct ReflectedFriend
{
    ReflectedFriend(int i, bool b) : i{i}, b{b} {}

private:
    int i;
    bool b;

    KL_REFLECT_STRUCT_FRIEND(ReflectedFriend, i, b)
};

struct ReflectedFriendDerived : ReflectedFriend
{
    ReflectedFriendDerived() : ReflectedFriend{7, false} {}

private:
    std::string s{"derived"};

    KL_REFLECT_STRUCT_DERIVED_FRIEND(ReflectedFriendDerived, ReflectedFriend, s)
};

// Minimal field_factory implementation for testing generated reflect_struct_fields() from use of
// KL_REFLECT_STRUCT* macros. Not intended for actual use.
template <typename Object>
class field_factory
{
    template <auto Ptr>
    class field_desc
    {
    public:
        constexpr field_desc(Object& object, const char* name) : name_{name}, object_{&object} {}
        constexpr const char* name() const noexcept { return name_; }
        constexpr decltype(auto) value() const { return ((*object_).*Ptr); }

    private:
        const char* name_;
        Object* object_;
    };

public:
    constexpr explicit field_factory(Object& object) noexcept : object_{object} {}

    template <auto Ptr>
    constexpr auto field(const char* name) const
    {
        return field_desc<Ptr>{object_, name};
    }

private:
    Object& object_;
};

template <typename Visitor, typename Self, typename Record>
constexpr void reflect_test(Visitor&& v, Self&& self, kl::ctti::record_class<Record>)
{
    reflect_struct_fields(field_factory{self}, std::forward<Visitor>(v),
                          kl::ctti::record<Record>);
}

} // namespace

TEST_CASE("reflect struct")
{
    using namespace std::string_literals;

    A a{2, true, 3.14};
    std::ostringstream ss;
    reflect_test(
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
    reflect_test(
        [&](auto field) {
            ++acc;
            last_name = field.name();
        },
        b, kl::ctti::record<B>);
    CHECK(acc == 4);
    CHECK(last_name == "ull");
}

TEST_CASE("reflect struct using hidden friends")
{
    ReflectedFriend reflected{42, true};

    std::ostringstream ss;
    reflect_test([&](auto field) {
        ss << field.name() << ": " << field.value() << "\n";
    }, reflected, kl::ctti::record<ReflectedFriend>);

    CHECK(ss.str() == "i: 42\nb: 1\n");
    CHECK(reflect_num_fields(kl::ctti::record<ReflectedFriend>) == 2);
}

TEST_CASE("reflect derived struct using hidden friends")
{
    ReflectedFriendDerived reflected;

    std::ostringstream ss;
    reflect_test([&](auto field) {
        ss << field.name() << ": " << field.value() << "\n";
    }, reflected, kl::ctti::record<ReflectedFriendDerived>);

    CHECK(ss.str() == "i: 7\nb: 0\ns: derived\n");
    CHECK(reflect_num_fields(kl::ctti::record<ReflectedFriendDerived>) == 3);
}
