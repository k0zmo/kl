#include "kl/resource.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/json.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

struct A
{
    int i;
    bool b;
    double d;
    std::string str;
};
KL_REFLECT_STRUCT(A, i, b, d, str)

struct B
{
    int i;
    bool b;
    std::string str;
    A a;
};
KL_REFLECT_STRUCT(B, i, b, str, a)

template <typename T>
auto dump_at_path(const T& obj, kl::resources::path_view path)
{
    return kl::resources::visit_at_path<std::string>(obj, path, [&](auto field) {
        return kl::json::dump(field.value());
    });
}

} // namespace

TEST_CASE("resource", "[resource]")
{
    B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
    auto res = kl::resources::make_resource(b);

    CHECK(dump_at_path(res.value, {}) ==
          R"({"i":1,"b":false,"str":"Test","a":{"i":42,"b":true,"d":3.14,"str":"Hello world!"}})");
    CHECK(dump_at_path(res.value, {"str"}) == R"("Test")");
    CHECK(dump_at_path(res.value, {"a"}) == R"({"i":42,"b":true,"d":3.14,"str":"Hello world!"})");
    CHECK(dump_at_path(res.value, {"a", "i"}) == "42");
    CHECK(dump_at_path(res.value, {"a", "str"}) == "\"Hello world!\"");
    CHECK_THROWS_AS(dump_at_path(res.value, {"a", "str1"}),
                    kl::resources::path_segment_not_found_error);
    CHECK_THROWS_AS(dump_at_path(res.value, {"c"}), kl::resources::path_segment_not_found_error);
    CHECK_THROWS_AS(dump_at_path(res.value, {"a", "str", "x"}),
                    kl::resources::path_not_traversable_error);
}
