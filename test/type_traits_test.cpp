#include "kl/type_traits.hpp"

#include <catch/catch.hpp>

TEST_CASE("type_traits")
{
    SECTION("at_type")
    {
        static_assert(
            std::is_same<kl::at_type_t<0, int, bool, const int>, int>::value,
            "???");
        static_assert(
            std::is_same<kl::at_type_t<1, int, bool, const int>, bool>::value,
            "???");
        static_assert(std::is_same<kl::at_type_t<2, int, bool, const int>,
                                   const int>::value,
                      "???");
    }

    SECTION("func_traits - lambda")
    {
        auto lambda = [](int, const bool&) { return 1.0f; };
        static_assert(
            std::is_same<kl::func_traits<decltype(lambda)>::return_type,
                         float>::value,
            "???");
        static_assert(
            std::is_same<kl::func_traits<decltype(lambda)>::class_type,
                         decltype(lambda)>::value,
            "???");
        static_assert(kl::func_traits<decltype(lambda)>::arity == 2, "???");
        static_assert(
            std::is_same<
                kl::func_traits<decltype(lambda)>::template arg<0>::type,
                int>::value,
            "???");
        static_assert(
            std::is_same<
                kl::func_traits<decltype(lambda)>::template arg<1>::type,
                const bool&>::value,
            "???");

        static_assert(kl::has_call_operator<decltype(lambda)>::value, "???");
    }

    SECTION("func_traits - member function pointer")
    {
        struct s { void foo(int) const {} };

        static_assert(
            std::is_same<kl::func_traits<decltype(&s::foo)>::return_type,
                         void>::value,
            "???");
        static_assert(
            std::is_same<kl::func_traits<decltype(&s::foo)>::class_type,
                         s>::value,
            "???");
        static_assert(kl::func_traits<decltype(&s::foo)>::arity == 1, "???");
        static_assert(
            std::is_same<
                kl::func_traits<decltype(&s::foo)>::template arg<0>::type,
                int>::value,
            "???");
    }

    SECTION("func_traits - functor")
    {
        struct op { bool operator()(double d) { return d < 0.0; } };

        static_assert(
            std::is_same<kl::func_traits<op>::return_type, bool>::value, "???");
        static_assert(std::is_same<kl::func_traits<op>::class_type, op>::value,
                      "???");
        static_assert(kl::func_traits<op>::arity == 1, "???");
        static_assert(std::is_same<kl::func_traits<op>::template arg<0>::type,
                                   double>::value,
                      "???");

        static_assert(kl::has_call_operator<op>::value, "???");
    }

    SECTION("is_same")
    {
        static_assert(kl::is_same<void, void, void, void>::value, "???");
        static_assert(!kl::is_same<void, bool, void>::value, "???");
    }

    SECTION("conjunction && disjunction")
    {
        static_assert(!kl::conjunction<kl::is_same<void, void>,
                                       kl::is_same<void, bool>>::value,
                      "???");
        static_assert(kl::disjunction<kl::is_same<void, void>,
                                      kl::is_same<void, bool>>::value,
                      "???");
    }
}
