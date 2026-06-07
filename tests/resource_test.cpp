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
auto dump_json(const T& obj, kl::resources::path_view path)
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

    CHECK(dump_json(res.value, {}) ==
          R"({"i":1,"b":false,"str":"Test","a":{"i":42,"b":true,"d":3.14,"str":"Hello world!"}})");
    CHECK(dump_json(res.value, {"str"}) == R"("Test")");
    CHECK(dump_json(res.value, {"a"}) == R"({"i":42,"b":true,"d":3.14,"str":"Hello world!"})");
    CHECK(dump_json(res.value, {"a", "i"}) == "42");
    CHECK(dump_json(res.value, {"a", "str"}) == "\"Hello world!\"");
    CHECK_THROWS_AS(dump_json(res.value, {"a", "str1"}),
                    kl::resources::path_segment_not_found_error);
    CHECK_THROWS_AS(dump_json(res.value, {"c"}), kl::resources::path_segment_not_found_error);
    CHECK_THROWS_AS(dump_json(res.value, {"a", "str", "x"}),
                    kl::resources::path_not_traversable_error);

    kl::json::owning_serialize_context serctx;
    auto json = kl::resources::serialize_at_path(res.value, {"a", "str"}, serctx);
    CHECK(kl::json::dump(json) == "\"Hello world!\"");

    auto a1j = R"({"i":13,"b":false,"d":2.72,"str":"byebye"})"_json;
    kl::json::deserialize_context dectx;
    kl::resources::deserialize_at_path(res.value, {"a"}, a1j, dectx);
    CHECK(res.value.a.i == 13);
    CHECK(res.value.a.b == false);
    CHECK(res.value.a.d == 2.72);
    CHECK(res.value.a.str == "byebye");
    CHECK(res.value.i == 1);
    CHECK(res.value.b == false);
    CHECK(res.value.str == "Test");

    auto strj = R"("updated")"_json;
    kl::resources::deserialize_at_path(res.value, {"a", "str"}, strj, dectx);
    CHECK(res.value.a.str == "updated");
    CHECK(dump_json(res.value, {"a", "str"}) == R"("updated")");
}

TEST_CASE("resource modification tracker marks paths and ancestors", "[resource]")
{
    kl::resources::modification_tracker tracker;

    const auto never_changed = kl::resources::modification_tracker::generation{0};
    CHECK(tracker.current() == kl::resources::modification_tracker::generation{1});
    CHECK(tracker.changed_at({}) == never_changed);
    CHECK(tracker.changed_at({"settings", "video"}) == never_changed);
    CHECK_FALSE(tracker.changed_since({"settings", "video"}, never_changed));

    const auto width_changed = tracker.notify_changed({"settings", "video", "width"});
    CHECK(width_changed == kl::resources::modification_tracker::generation{2});
    CHECK(tracker.current() == width_changed);

    CHECK(tracker.changed_at({}) == width_changed);
    CHECK(tracker.changed_at({"settings"}) == width_changed);
    CHECK(tracker.changed_at({"settings", "video"}) == width_changed);
    CHECK(tracker.changed_at({"settings", "video", "width"}) == width_changed);

    CHECK(tracker.changed_at({"settings", "video", "height"}) == never_changed);
    CHECK(tracker.changed_at({"settings", "audio"}) == never_changed);
    CHECK(tracker.changed_since({"settings", "video"}, never_changed));
    CHECK_FALSE(tracker.changed_since({"settings", "video"}, width_changed));
}

TEST_CASE("resource modification tracker handles subtree replacement", "[resource]")
{
    kl::resources::modification_tracker tracker;

    const auto width_changed = tracker.notify_changed({"settings", "video", "width"});
    const auto video_replaced = tracker.notify_changed({"settings", "video"}, true);

    CHECK(video_replaced == kl::resources::modification_tracker::generation{3});
    CHECK(tracker.changed_at({}) == video_replaced);
    CHECK(tracker.changed_at({"settings"}) == video_replaced);
    CHECK(tracker.changed_at({"settings", "video"}) == video_replaced);

    CHECK(tracker.changed_at({"settings", "video", "width"}) == video_replaced);
    CHECK(tracker.changed_at({"settings", "video", "height"}) == video_replaced);
    CHECK(tracker.changed_at({"settings", "video", "nested", "leaf"}) == video_replaced);

    CHECK(tracker.changed_at({"settings", "audio"}) ==
          kl::resources::modification_tracker::generation{0});
    CHECK(tracker.changed_since({"settings", "video", "width"}, width_changed));
}
