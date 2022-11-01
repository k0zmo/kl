#include "kl/tuple.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <vector>

TEST_CASE("tuple")
{
    SECTION("apply_fn")
    {
        std::tuple<int, double> t{1, 2.0};
        kl::tuple::apply_fn::call(t, [](int& i, double& d) {
            i *= 2; d *= 2;
        });
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == 4.0);
    }

    SECTION("transform_ref_fn")
    {
        int arr[3]{10, 11, 12};
        std::vector<short> vs = {4, 5, 6};
        std::tuple<int*, std::optional<double>,
                   std::vector<short>::const_iterator>
            t{std::begin(arr), 3.0, vs.cbegin() + 1};

        auto tt = kl::tuple::transform_ref_fn::call(t);

        REQUIRE(std::get<0>(tt) == 10);
        REQUIRE(std::get<1>(tt) == 3.0);
        REQUIRE(std::get<2>(tt) == 5);

        static_assert(
            std::is_same<std::tuple_element_t<0, decltype(tt)>, int&>::value);
        static_assert(std::is_same<std::tuple_element_t<1, decltype(tt)>,
                                   double&>::value);
        static_assert(std::is_same<std::tuple_element_t<2, decltype(tt)>,
                                   const short&>::value);
        static_assert(std::tuple_size<decltype(tt)>::value == 3);
    }

    SECTION("transform_deref_fn")
    {
        int arr[3]{10, 11, 12};
        std::vector<short> vs = {4, 5, 6};
        std::tuple<int*, std::optional<double>,
                   std::vector<short>::const_iterator>
            t{std::begin(arr), 3.0, vs.cbegin() + 1};

        auto tt = kl::tuple::transform_deref_fn::call(t);

        REQUIRE(std::get<0>(tt) == 10);
        REQUIRE(std::get<1>(tt) == 3.0);
        REQUIRE(std::get<2>(tt) == 5);

        static_assert(
            std::is_same<std::tuple_element_t<0, decltype(tt)>, int>::value);
        static_assert(
            std::is_same<std::tuple_element_t<1, decltype(tt)>, double>::value);
        static_assert(
            std::is_same<std::tuple_element_t<2, decltype(tt)>, short>::value);
        static_assert(std::tuple_size<decltype(tt)>::value == 3);
    }

    SECTION("for_each_fn")
    {
        int arr[3]{};
        std::tuple<int, int*> t{1, std::begin(arr)};
        kl::tuple::for_each_fn::call(t, [](auto& f) { ++f; });
        REQUIRE(std::get<0>(t) == 2);
        REQUIRE(std::get<1>(t) == std::begin(arr) + 1);
    }

    SECTION("not_equal_fn")
    {
        std::tuple<int, bool> t1{10, true}, t2{9, false};
        REQUIRE(kl::tuple::not_equal_fn::call(t1, t2));
        std::get<1>(t2) = true;
        REQUIRE(!kl::tuple::not_equal_fn::call(t1, t2));
        std::get<1>(t2) = false;
        REQUIRE(kl::tuple::not_equal_fn::call(t1, t2));
        std::get<0>(t2) = 10;
        REQUIRE(!kl::tuple::not_equal_fn::call(t1, t2));
    }

    SECTION("distance_fn")
    {
        int arr[] = {1,2,3};
        std::vector<int> vec = {4,5};

        using tuple_type = std::tuple<int *, std::vector<int>::iterator>;
        tuple_type t1{std::begin(arr), std::begin(vec)},
                   t2{std::end(arr), std::end(vec)};

        REQUIRE(kl::tuple::distance_fn::call(t1, t2) == 2);

        vec.push_back(6);
        t1 = tuple_type{std::begin(arr), std::begin(vec)};
        t2 = tuple_type{std::end(arr), std::end(vec)};
        REQUIRE(kl::tuple::distance_fn::call(t1, t2) == 3);

        vec.push_back(7);
        t1 = tuple_type{std::begin(arr), std::begin(vec)};
        t2 = tuple_type{std::end(arr), std::end(vec)};
        REQUIRE(kl::tuple::distance_fn::call(t1, t2) == 3);
    }

    SECTION("all_true_fn")
    {
        std::tuple<bool, int, std::optional<int>> t{true, 1, std::nullopt};
        REQUIRE(!kl::tuple::all_true_fn::call(t));
        std::get<2>(t) = 2;
        REQUIRE(kl::tuple::all_true_fn::call(t));
        std::get<1>(t) = 0;
        REQUIRE(!kl::tuple::all_true_fn::call(t));
    }
}
