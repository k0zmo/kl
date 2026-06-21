#include "kl/resource.hpp"
#include "kl/resource_attributes.hpp"
#include "kl/resource_op.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/json.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace attr = kl::resources::attributes;

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

struct AccessInner
{
    int value;
    int read_only_value;

    bool _validate_true = true;

    template <typename Context>
    void validate_resource(Context&) const
    {
        if (!_validate_true)
            throw kl::resources::validation_error("validate failed");
    }
};
KL_REFLECT_STRUCT(AccessInner,
                  value,
                  (read_only_value, attr::read_only))

struct AccessRoot
{
    int value;
    AccessInner inner;
    AccessInner read_only_inner;
    AccessInner subtree_read_only_inner;
    AccessInner fully_read_only_inner;
};
KL_REFLECT_STRUCT(AccessRoot,
                  value,
                  inner,
                  (read_only_inner, attr::read_only),
                  (subtree_read_only_inner, attr::subtree_read_only),
                  (fully_read_only_inner, attr::read_only, attr::subtree_read_only))

struct OptionalRoot
{
    std::optional<int> maybe_value;
    std::optional<AccessInner> maybe_inner;
    int value;
    std::optional<int> read_only_maybe_value;
};
KL_REFLECT_STRUCT(OptionalRoot,
                  maybe_value,
                  maybe_inner,
                  value,
                  (read_only_maybe_value, attr::read_only))

struct MemberValidated
{
    int min;
    int max;

    void validate_resource(kl::json::deserialize_context&) const
    {
        if (min > max)
            throw kl::resources::validation_error{"min must be less than or equal to max"};
    }
};
KL_REFLECT_STRUCT(MemberValidated, min, max)

struct OptionalValidatedRoot
{
    std::optional<MemberValidated> maybe_limits;
};
KL_REFLECT_STRUCT(OptionalValidatedRoot, maybe_limits)

struct NestedValidatedRoot
{
    MemberValidated limits;
};
KL_REFLECT_STRUCT(NestedValidatedRoot, limits)

struct SizeValidatedRoot
{
    std::vector<A> items;

    void validate_resource(kl::json::deserialize_context&) const
    {
        if (items.size() > 2)
            throw kl::resources::validation_error{"too many items"};
    }
};
KL_REFLECT_STRUCT(SizeValidatedRoot, items)

struct MinSizeValidatedRoot
{
    std::vector<A> items;

    void validate_resource(kl::json::deserialize_context&) const
    {
        if (items.size() < 2)
            throw kl::resources::validation_error{"too few items"};
    }
};
KL_REFLECT_STRUCT(MinSizeValidatedRoot, items)

struct RequiredOptionalRoot
{
    std::optional<int> required_value;

    void validate_resource(kl::json::deserialize_context&) const
    {
        if (!required_value)
            throw kl::resources::validation_error{"required_value cannot be null"};
    }
};
KL_REFLECT_STRUCT(RequiredOptionalRoot, required_value)

struct CollectionRoot
{
    std::vector<A> items;
    std::vector<int> values;
    std::vector<A> read_only_items{};
    std::vector<A> subtree_read_only_items{};
};
KL_REFLECT_STRUCT(CollectionRoot,
                  items,
                  values,
                  (read_only_items, attr::read_only),
                  (subtree_read_only_items, attr::subtree_read_only))

struct CollectionAccessInner
{
    std::vector<A> items;
};
KL_REFLECT_STRUCT(CollectionAccessInner, items)

struct CollectionAccessRoot
{
    CollectionAccessInner subtree_read_only_inner;
};
KL_REFLECT_STRUCT(CollectionAccessRoot,
                  (subtree_read_only_inner, attr::subtree_read_only))

struct IntKeyedItem
{
    int id;
    std::string name;
    std::optional<std::string> note;
};
KL_REFLECT_STRUCT(IntKeyedItem, id, name, note)

struct KeyedCollectionRoot
{
    std::vector<A> string_items;
    std::vector<IntKeyedItem> int_items;
};
KL_REFLECT_STRUCT(KeyedCollectionRoot,
                  (string_items, attr::child_key<&A::str>),
                  (int_items, attr::child_key<&IntKeyedItem::id>))

bool resource_key_matches(const IntKeyedItem&, int key, std::string_view segment)
{
    std::size_t index = 0;
    if (!kl::resources::detail::parse_index(segment, index))
        return false;

    return key == static_cast<int>(index);
}

struct AdlValidated
{
    int value;
};
KL_REFLECT_STRUCT(AdlValidated, value)

void validate_resource(const AdlValidated& value, kl::json::deserialize_context&)
{
    if (value.value == 13)
        throw kl::resources::validation_error{"13 is not allowed"};
}

template <typename T>
auto dump_json(const T& obj, kl::resources::path_view path)
{
    return kl::resources::visit_at_path<std::string>(
        obj, path,
        [&](auto field, [[maybe_unused]] auto& ctx) { return kl::json::dump(field.value()); });
}

} // namespace

TEST_CASE("resource", "[resource]")
{
    B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
    auto res = kl::resources::make_resource(b);

    SECTION("dumps values at path")
    {
        CHECK(dump_json(res.value, {}) ==
              R"({"i":1,"b":false,"str":"Test","a":{"i":42,"b":true,"d":3.14,"str":"Hello world!"}})");
        CHECK(dump_json(res.value, {"str"}) == R"("Test")");
        CHECK(dump_json(res.value, {"a"}) ==
              R"({"i":42,"b":true,"d":3.14,"str":"Hello world!"})");
        CHECK(dump_json(res.value, {"a", "i"}) == "42");
        CHECK(dump_json(res.value, {"a", "str"}) == "\"Hello world!\"");
    }

    SECTION("reports invalid paths")
    {
        CHECK_THROWS_AS(dump_json(res.value, {"a", "str1"}),
                        kl::resources::path_segment_not_found_error);
        CHECK_THROWS_AS(dump_json(res.value, {"c"}),
                        kl::resources::path_segment_not_found_error);
        CHECK_THROWS_AS(dump_json(res.value, {"a", "str", "x"}),
                        kl::resources::path_not_traversable_error);
    }

    SECTION("serializes value at path")
    {
        kl::json::owning_serialize_context serctx;
        auto json = kl::resources::serialize_at_path(res.value, {"a", "str"}, serctx);
        CHECK(kl::json::dump(json) == "\"Hello world!\"");
    }

    SECTION("deserializes value at path")
    {
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
}

TEST_CASE("resource get", "[resource]")
{
    B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
    auto res = kl::resources::make_resource(b);
    auto dumper = [](const auto& value) { return kl::json::dump(value); };

    SECTION("returns serialized value at path")
    {
        const auto root = kl::resources::get(res, {}, dumper);
        CHECK(root.status == kl::resources::status_code::ok);
        CHECK(root.body ==
              R"({"i":1,"b":false,"str":"Test","a":{"i":42,"b":true,"d":3.14,"str":"Hello world!"}})");

        const auto nested = kl::resources::get(res, {"a", "str"}, dumper);
        CHECK(nested.status == kl::resources::status_code::ok);
        CHECK(nested.body == R"("Hello world!")");
    }

    SECTION("returns not found for missing path")
    {
        const auto result = kl::resources::get(res, {"a", "missing"}, dumper);

        CHECK(result.status == kl::resources::status_code::not_found);
        CHECK(result.body.empty());
    }

    SECTION("returns not found when path is not traversable")
    {
        const auto result = kl::resources::get(res, {"a", "str", "x"}, dumper);

        CHECK(result.status == kl::resources::status_code::not_found);
        CHECK(result.body.empty());
    }
}

TEST_CASE("resource indexed collection traversal", "[resource]")
{
    CollectionRoot root{
        {
            A{1, true, 1.5, "first"},
            A{2, false, 2.5, "second"},
        },
        {7, 8, 9},
    };
    auto res = kl::resources::make_resource(root);
    auto dumper = [](const auto& value) { return kl::json::dump(value); };

    SECTION("serializes collection element by index")
    {
        CHECK(dump_json(res.value, {"items", "0"}) ==
              R"({"i":1,"b":true,"d":1.5,"str":"first"})");
        CHECK(dump_json(res.value, {"items", "1", "str"}) == R"("second")");
        CHECK(dump_json(res.value, {"values", "2"}) == "9");

        kl::json::owning_serialize_context ctx;
        auto json = kl::resources::serialize_at_path(res.value, {"items", "1", "i"}, ctx);
        CHECK(kl::json::dump(json) == "2");
    }

    SECTION("get returns collection element by index")
    {
        const auto element = kl::resources::get(res, {"items", "0"}, dumper);
        CHECK(element.status == kl::resources::status_code::ok);
        CHECK(element.body == R"({"i":1,"b":true,"d":1.5,"str":"first"})");

        const auto leaf = kl::resources::get(res, {"items", "1", "str"}, dumper);
        CHECK(leaf.status == kl::resources::status_code::ok);
        CHECK(leaf.body == R"("second")");
    }

    SECTION("reports invalid collection index paths")
    {
        CHECK_THROWS_AS(dump_json(res.value, {"items", "2"}),
                        kl::resources::path_segment_not_found_error);
        CHECK_THROWS_AS(dump_json(res.value, {"items", "abc"}),
                        kl::resources::path_segment_not_found_error);
        CHECK_THROWS_AS(dump_json(res.value, {"values", "0", "x"}),
                        kl::resources::path_not_traversable_error);

        const auto missing = kl::resources::get(res, {"items", "2"}, dumper);
        CHECK(missing.status == kl::resources::status_code::not_found);
        CHECK(missing.body.empty());
    }
}

TEST_CASE("resource keyed collection traversal", "[resource]")
{
    KeyedCollectionRoot root{
        {
            A{1, true, 1.5, "first"},
            A{2, false, 2.5, "second"},
        },
        {
            IntKeyedItem{7, "seven", "lucky"},
            IntKeyedItem{13, "thirteen", "prime"},
        },
    };
    auto res = kl::resources::make_resource(root);
    auto dumper = [](const auto& value) { return kl::json::dump(value); };

    SECTION("serializes collection element by key")
    {
        CHECK(dump_json(res.value, {"string_items", "second"}) ==
              R"({"i":2,"b":false,"d":2.5,"str":"second"})");
        CHECK(dump_json(res.value, {"string_items", "second", "i"}) == "2");
    }

    SECTION("get returns collection element by key")
    {
        const auto element = kl::resources::get(res, {"string_items", "first"}, dumper);
        CHECK(element.status == kl::resources::status_code::ok);
        CHECK(element.body == R"({"i":1,"b":true,"d":1.5,"str":"first"})");

        const auto leaf =
            kl::resources::get(res, {"string_items", "second", "str"}, dumper);
        CHECK(leaf.status == kl::resources::status_code::ok);
        CHECK(leaf.body == R"("second")");
    }

    SECTION("put updates collection element by key")
    {
        kl::json::deserialize_context ctx;
        auto value = R"(42)"_json;

        const auto result =
            kl::resources::put(res, {"string_items", "second", "i"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::ok);
        CHECK(result.body.empty());
        CHECK(res.value.string_items[0].i == 1);
        CHECK(res.value.string_items[1].i == 42);
        CHECK(res.state.modifications.changed_at({"string_items", "second", "i"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("put rejects changing a child key")
    {
        kl::json::deserialize_context ctx;
        auto value = R"("first")"_json;

        const auto result =
            kl::resources::put(res, {"string_items", "second", "str"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.string_items[0].str == "first");
        CHECK(res.value.string_items[1].str == "second");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("put rejects changing a child key through element replacement")
    {
        kl::json::deserialize_context ctx;
        auto value = R"({"i":2,"b":false,"d":2.5,"str":"renamed"})"_json;

        const auto result =
            kl::resources::put(res, {"string_items", "second"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.string_items[1].str == "second");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("put replaces a keyed element when its key is unchanged")
    {
        kl::json::deserialize_context ctx;
        auto value = R"({"i":42,"b":true,"d":4.5,"str":"second"})"_json;

        const auto result =
            kl::resources::put(res, {"string_items", "second"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::ok);
        CHECK(result.body.empty());
        CHECK(res.value.string_items[1].i == 42);
        CHECK(res.value.string_items[1].str == "second");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("keyed collections do not fall back to index traversal")
    {
        CHECK_THROWS_AS(dump_json(res.value, {"string_items", "0"}),
                        kl::resources::path_segment_not_found_error);

        const auto missing = kl::resources::get(res, {"string_items", "0"}, dumper);
        CHECK(missing.status == kl::resources::status_code::not_found);
        CHECK(missing.body.empty());
    }

    SECTION("get returns collection element through ADL key matcher")
    {
        const auto element = kl::resources::get(res, {"int_items", "13"}, dumper);
        CHECK(element.status == kl::resources::status_code::ok);
        CHECK(element.body == R"({"id":13,"name":"thirteen","note":"prime"})");

        const auto leaf = kl::resources::get(res, {"int_items", "7", "name"}, dumper);
        CHECK(leaf.status == kl::resources::status_code::ok);
        CHECK(leaf.body == R"("seven")");
    }

    SECTION("delete clears nullable field through ADL key matcher")
    {
        kl::json::deserialize_context ctx;

        const auto result = kl::resources::del(res, {"int_items", "13", "note"}, ctx);

        CHECK(result.status == kl::resources::status_code::no_content);
        CHECK(result.body.empty());
        REQUIRE(res.value.int_items[0].note.has_value());
        CHECK_FALSE(res.value.int_items[1].note.has_value());
        CHECK(res.state.modifications.changed_at({"int_items", "13", "note"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("get returns not found when custom matcher rejects the segment")
    {
        const auto result = kl::resources::get(res, {"int_items", "not-an-int"}, dumper);

        CHECK(result.status == kl::resources::status_code::not_found);
        CHECK(result.body.empty());
    }
}

TEST_CASE("resource put", "[resource]")
{
    kl::json::deserialize_context ctx;

    SECTION("updates writable scalar path")
    {
        B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
        auto res = kl::resources::make_resource(b);
        auto value = R"(99)"_json;

        const auto result = kl::resources::put(res, {"a", "i"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::ok);
        CHECK(result.body.empty());
        CHECK(res.value.a.i == 99);
        CHECK(res.state.modifications.changed_at({"a", "i"}) ==
              kl::resources::modification_tracker::generation{2});
        CHECK(res.state.modifications.changed_at({"a"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("marks reflectable path replacements as subtree changes")
    {
        AccessRoot root{};
        auto res = kl::resources::make_resource(root);
        auto value = R"({"value":7,"read_only_value":8})"_json;

        const auto result = kl::resources::put(res, {"inner"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::ok);
        CHECK(result.body.empty());
        CHECK(res.value.inner.value == 7);
        CHECK(res.value.inner.read_only_value == 8);
        CHECK(res.state.modifications.changed_at({"inner", "value"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("replaces root and marks subtree changed")
    {
        B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
        auto res = kl::resources::make_resource(b);
        auto value =
            R"({"i":2,"b":true,"str":"Root","a":{"i":13,"b":false,"d":2.72,"str":"Nested"}})"_json;

        const auto result = kl::resources::put(res, {}, value, ctx);

        CHECK(result.status == kl::resources::status_code::ok);
        CHECK(result.body.empty());
        CHECK(res.value.i == 2);
        CHECK(res.value.b);
        CHECK(res.value.str == "Root");
        CHECK(res.value.a.i == 13);
        CHECK_FALSE(res.value.a.b);
        CHECK(res.value.a.d == 2.72);
        CHECK(res.value.a.str == "Nested");
        CHECK(res.state.modifications.changed_at({}) ==
              kl::resources::modification_tracker::generation{2});
        CHECK(res.state.modifications.changed_at({"a", "str"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("rejects read-only paths")
    {
        AccessRoot root{};
        root.inner.read_only_value = 4;
        auto res = kl::resources::make_resource(root);
        auto value = R"(9)"_json;

        const auto result =
            kl::resources::put(res, {"inner", "read_only_value"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.inner.read_only_value == 4);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects subtree read-only descendants")
    {
        AccessRoot root{};
        root.subtree_read_only_inner.value = 4;
        auto res = kl::resources::make_resource(root);
        auto value = R"(9)"_json;

        const auto result =
            kl::resources::put(res, {"subtree_read_only_inner", "value"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.subtree_read_only_inner.value == 4);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects descendants of subtree read-only collection elements")
    {
        CollectionRoot root{
            {},
            {},
            {},
            {A{4, true, 1.5, "first"}},
        };
        auto res = kl::resources::make_resource(root);
        auto value = R"(9)"_json;

        const auto result =
            kl::resources::put(res, {"subtree_read_only_items", "0", "i"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.subtree_read_only_items[0].i == 4);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns not found for missing path")
    {
        AccessRoot root{};
        root.value = 3;
        auto res = kl::resources::make_resource(root);
        auto value = R"(9)"_json;

        const auto result = kl::resources::put(res, {"missing"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::not_found);
        CHECK(result.body.empty());
        CHECK(res.value.value == 3);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns not found when path is not traversable")
    {
        B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
        auto res = kl::resources::make_resource(b);
        auto value = R"(9)"_json;

        const auto result = kl::resources::put(res, {"a", "str", "x"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::not_found);
        CHECK(result.body.empty());
        CHECK(res.value.a.str == "Hello world!");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns bad request when value cannot be deserialized")
    {
        B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
        auto res = kl::resources::make_resource(b);
        auto value = R"("not an int")"_json;

        const auto result = kl::resources::put(res, {"a", "i"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::bad_request);
        CHECK(result.body.empty());
        CHECK(res.value.a.i == 42);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("preserves old value when partial deserialization fails")
    {
        B b{1, false, "Test", {42, true, 3.14, "Hello world!"}};
        auto res = kl::resources::make_resource(b);
        auto value = R"({"i":13,"b":"not a bool","d":2.72,"str":"updated"})"_json;

        const auto result = kl::resources::put(res, {"a"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::bad_request);
        CHECK(result.body.empty());
        CHECK(res.value.a.i == 42);
        CHECK(res.value.a.b);
        CHECK(res.value.a.d == 3.14);
        CHECK(res.value.a.str == "Hello world!");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when member validation fails")
    {
        auto res = kl::resources::make_resource(MemberValidated{1, 5});
        auto value = R"({"min":10,"max":2})"_json;

        const auto result = kl::resources::put(res, {}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "min must be less than or equal to max");
        CHECK(res.value.min == 1);
        CHECK(res.value.max == 5);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when leaf update breaks root validation")
    {
        auto res = kl::resources::make_resource(MemberValidated{1, 5});
        auto value = R"(10)"_json;

        const auto result = kl::resources::put(res, {"min"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "min must be less than or equal to max");
        CHECK(res.value.min == 1);
        CHECK(res.value.max == 5);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when nested leaf update breaks child validation")
    {
        auto res = kl::resources::make_resource(NestedValidatedRoot{{1, 5}});
        auto value = R"(10)"_json;

        const auto result = kl::resources::put(res, {"limits", "min"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "min must be less than or equal to max");
        CHECK(res.value.limits.min == 1);
        CHECK(res.value.limits.max == 5);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when optional contained member validation fails")
    {
        auto res =
            kl::resources::make_resource(OptionalValidatedRoot{MemberValidated{1, 5}});
        auto value = R"({"min":10,"max":2})"_json;

        const auto result = kl::resources::put(res, {"maybe_limits"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "min must be less than or equal to max");
        REQUIRE(res.value.maybe_limits.has_value());
        CHECK(res.value.maybe_limits->min == 1);
        CHECK(res.value.maybe_limits->max == 5);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when ADL validation fails")
    {
        auto res = kl::resources::make_resource(AdlValidated{7});
        auto value = R"({"value":13})"_json;

        const auto result = kl::resources::put(res, {}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "13 is not allowed");
        CHECK(res.value.value == 7);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }
}

TEST_CASE("resource post", "[resource]")
{
    kl::json::deserialize_context ctx;

    SECTION("appends to unkeyed collection")
    {
        CollectionRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
            {7, 8, 9},
        };
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":3,"b":true,"d":3.5,"str":"third"})"_json;

        const auto result = kl::resources::post(res, {"items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::created);
        CHECK(result.body.empty());
        REQUIRE(res.value.items.size() == 3);
        CHECK(res.value.items[2].i == 3);
        CHECK(res.value.items[2].str == "third");
        CHECK(res.state.modifications.changed_at({"items"}) ==
              kl::resources::modification_tracker::generation{2});
        CHECK(res.state.modifications.changed_at({"items", "2", "str"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("appends to keyed collection")
    {
        KeyedCollectionRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
            {
                IntKeyedItem{7, "seven", "lucky"},
                IntKeyedItem{13, "thirteen", "prime"},
            },
        };
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":3,"b":true,"d":3.5,"str":"third"})"_json;

        const auto result = kl::resources::post(res, {"string_items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::created);
        CHECK(result.body.empty());
        REQUIRE(res.value.string_items.size() == 3);
        CHECK(res.value.string_items[2].str == "third");
        CHECK(res.state.modifications.changed_at({"string_items", "third", "i"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("rejects duplicate keyed children")
    {
        KeyedCollectionRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
            {
                IntKeyedItem{7, "seven", "lucky"},
                IntKeyedItem{13, "thirteen", "prime"},
            },
        };
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":9,"b":true,"d":9.5,"str":"second"})"_json;

        const auto result = kl::resources::post(res, {"string_items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "duplicate child key");
        REQUIRE(res.value.string_items.size() == 2);
        CHECK(res.value.string_items[1].i == 2);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when append breaks root validation")
    {
        SizeValidatedRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
        };
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":3,"b":true,"d":3.5,"str":"third"})"_json;

        const auto result = kl::resources::post(res, {"items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "too many items");
        REQUIRE(res.value.items.size() == 2);
        CHECK(res.value.items[0].str == "first");
        CHECK(res.value.items[1].str == "second");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns bad request when value cannot be deserialized")
    {
        CollectionRoot root{
            {
                A{1, true, 1.5, "first"},
            },
            {7, 8, 9},
        };
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":"bad","b":true,"d":3.5,"str":"third"})"_json;

        const auto result = kl::resources::post(res, {"items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::bad_request);
        CHECK(result.body.empty());
        REQUIRE(res.value.items.size() == 1);
        CHECK(res.value.items[0].str == "first");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects scalar paths")
    {
        AccessRoot root{};
        auto res = kl::resources::make_resource(root);
        auto value = R"(9)"_json;

        const auto result = kl::resources::post(res, {"value"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects read-only collections")
    {
        CollectionRoot root{};
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":3,"b":true,"d":3.5,"str":"third"})"_json;

        const auto result = kl::resources::post(res, {"read_only_items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.read_only_items.empty());
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects appending to subtree read-only collections")
    {
        CollectionRoot root{};
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":3,"b":true,"d":3.5,"str":"third"})"_json;

        const auto result =
            kl::resources::post(res, {"subtree_read_only_items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.subtree_read_only_items.empty());
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects descendants of subtree read-only fields")
    {
        CollectionAccessRoot root{};
        auto res = kl::resources::make_resource(root);
        auto value = R"({"i":3,"b":true,"d":3.5,"str":"third"})"_json;

        const auto result =
            kl::resources::post(res, {"subtree_read_only_inner", "items"}, value, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.subtree_read_only_inner.items.empty());
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }
}

TEST_CASE("resource delete", "[resource]")
{
    kl::json::deserialize_context ctx;

    SECTION("removes indexed collection element")
    {
        CollectionRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
            {7, 8, 9},
        };
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"items", "0"}, ctx);

        CHECK(result.status == kl::resources::status_code::no_content);
        CHECK(result.body.empty());
        REQUIRE(res.value.items.size() == 1);
        CHECK(res.value.items[0].str == "second");
        CHECK(res.state.modifications.changed_at({"items"}) ==
              kl::resources::modification_tracker::generation{2});
        CHECK(res.state.modifications.changed_at({"items", "0", "str"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("removes keyed collection element")
    {
        KeyedCollectionRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
            {
                IntKeyedItem{7, "seven", "lucky"},
                IntKeyedItem{13, "thirteen", "prime"},
            },
        };
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"string_items", "second"}, ctx);

        CHECK(result.status == kl::resources::status_code::no_content);
        CHECK(result.body.empty());
        REQUIRE(res.value.string_items.size() == 1);
        CHECK(res.value.string_items[0].str == "first");
        CHECK(res.state.modifications.changed_at({"string_items"}) ==
              kl::resources::modification_tracker::generation{2});
        CHECK(res.state.modifications.changed_at({"string_items", "first", "i"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("clears nullable scalar path")
    {
        OptionalRoot root{13, AccessInner{1, 2}, 42, 7};
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"maybe_value"}, ctx);

        CHECK(result.status == kl::resources::status_code::no_content);
        CHECK(result.body.empty());
        CHECK_FALSE(res.value.maybe_value.has_value());
        CHECK(res.state.modifications.changed_at({"maybe_value"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("clears nullable reflectable path and marks subtree changed")
    {
        OptionalRoot root{13, AccessInner{1, 2}, 42, 7};
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"maybe_inner"}, ctx);

        CHECK(result.status == kl::resources::status_code::no_content);
        CHECK(result.body.empty());
        CHECK_FALSE(res.value.maybe_inner.has_value());
        CHECK(res.state.modifications.changed_at({"maybe_inner"}) ==
              kl::resources::modification_tracker::generation{2});
        CHECK(res.state.modifications.changed_at({"maybe_inner", "value"}) ==
              kl::resources::modification_tracker::generation{2});
    }

    SECTION("validation rejects a deletion")
    {
        OptionalRoot root{13, AccessInner{1, 2}, 42, 7};
        auto res = kl::resources::make_resource(root);
        res.value.maybe_inner->_validate_true = false;

        const auto result = kl::resources::del(res, {"maybe_inner"}, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "validate failed");
        REQUIRE(res.value.maybe_inner.has_value());
        CHECK(res.value.maybe_inner->value == 1);
        CHECK(res.value.maybe_inner->read_only_value == 2);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects non-nullable paths")
    {
        OptionalRoot root{13, AccessInner{1, 2}, 42, 7};
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"value"}, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        CHECK(res.value.value == 42);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects read-only nullable paths")
    {
        OptionalRoot root{13, AccessInner{1, 2}, 42, 7};
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"read_only_maybe_value"}, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        REQUIRE(res.value.read_only_maybe_value.has_value());
        CHECK(*res.value.read_only_maybe_value == 7);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects read-only collection element deletion")
    {
        CollectionRoot root{
            {},
            {},
            {A{1, true, 1.5, "first"}},
        };
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"read_only_items", "0"}, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        REQUIRE(res.value.read_only_items.size() == 1);
        CHECK(res.value.read_only_items[0].str == "first");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("rejects subtree read-only collection element deletion")
    {
        CollectionRoot root{
            {},
            {},
            {},
            {A{1, true, 1.5, "first"}},
        };
        auto res = kl::resources::make_resource(root);

        const auto result =
            kl::resources::del(res, {"subtree_read_only_items", "0"}, ctx);

        CHECK(result.status == kl::resources::status_code::method_not_allowed);
        CHECK(result.body.empty());
        REQUIRE(res.value.subtree_read_only_items.size() == 1);
        CHECK(res.value.subtree_read_only_items[0].str == "first");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when deletion breaks root validation")
    {
        auto res = kl::resources::make_resource(RequiredOptionalRoot{13});

        const auto result = kl::resources::del(res, {"required_value"}, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "required_value cannot be null");
        REQUIRE(res.value.required_value.has_value());
        CHECK(*res.value.required_value == 13);
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns conflict when collection deletion breaks root validation")
    {
        MinSizeValidatedRoot root{
            {
                A{1, true, 1.5, "first"},
                A{2, false, 2.5, "second"},
            },
        };
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"items", "1"}, ctx);

        CHECK(result.status == kl::resources::status_code::conflict);
        CHECK(result.body == "too few items");
        REQUIRE(res.value.items.size() == 2);
        CHECK(res.value.items[0].str == "first");
        CHECK(res.value.items[1].str == "second");
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }

    SECTION("returns not found for missing path")
    {
        OptionalRoot root{13, AccessInner{1, 2}, 42, 7};
        auto res = kl::resources::make_resource(root);

        const auto result = kl::resources::del(res, {"missing"}, ctx);

        CHECK(result.status == kl::resources::status_code::not_found);
        CHECK(result.body.empty());
        CHECK(res.state.modifications.current() ==
              kl::resources::modification_tracker::generation{1});
    }
}

TEST_CASE("resource modification tracker", "[resource]")
{
    kl::resources::modification_tracker tracker;

    const auto never_changed = kl::resources::modification_tracker::generation{0};
    CHECK(tracker.current() == kl::resources::modification_tracker::generation{1});
    CHECK(tracker.changed_at({}) == never_changed);
    CHECK(tracker.changed_at({"settings", "video"}) == never_changed);
    CHECK_FALSE(tracker.changed_since({"settings", "video"}, never_changed));

    SECTION("marks paths and ancestors")
    {
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

        const auto audio_changed = tracker.notify_changed({"settings", "audio"});
        CHECK_FALSE(tracker.changed_since({"settings", "video", "width"}, audio_changed));
    }

    SECTION("handles subtree replacement")
    {
        const auto width_changed = tracker.notify_changed({"settings", "video", "width"});
        const auto video_replaced = tracker.notify_changed({"settings", "video"}, true);

        CHECK(video_replaced == kl::resources::modification_tracker::generation{3});
        CHECK(tracker.changed_at({}) == video_replaced);
        CHECK(tracker.changed_at({"settings"}) == video_replaced);
        CHECK(tracker.changed_at({"settings", "video"}) == video_replaced);

        CHECK(tracker.changed_at({"settings", "video", "width"}) == video_replaced);
        CHECK(tracker.changed_at({"settings", "video", "height"}) == video_replaced);
        CHECK(tracker.changed_at({"settings", "video", "nested", "leaf"}) ==
              video_replaced);

        CHECK(tracker.changed_at({"settings", "audio"}) ==
              kl::resources::modification_tracker::generation{0});
        CHECK(tracker.changed_since({"settings", "video", "width"}, width_changed));
    }
}

TEST_CASE("resource traversal reports write access constraints", "[resource]")
{
    AccessRoot root{};

    auto direct_write_forbidden_at_path = [](auto& obj, kl::resources::path_view path) {
        return kl::resources::visit_at_path<
            bool>(obj, path, [](auto node, kl::resources::access_context ctx) {
            return ctx.subtree_read_only ||
                   decltype(node)::template has_attribute<attr::read_only_t>();
        });
    };

    CHECK_FALSE(direct_write_forbidden_at_path(root, {}));
    CHECK_FALSE(direct_write_forbidden_at_path(root, {"value"}));
    CHECK_FALSE(direct_write_forbidden_at_path(root, {"inner"}));
    CHECK_FALSE(direct_write_forbidden_at_path(root, {"inner", "value"}));

    CHECK(direct_write_forbidden_at_path(root, {"inner", "read_only_value"}));

    CHECK(direct_write_forbidden_at_path(root, {"read_only_inner"}));
    CHECK_FALSE(direct_write_forbidden_at_path(root, {"read_only_inner", "value"}));

    CHECK_FALSE(direct_write_forbidden_at_path(root, {"subtree_read_only_inner"}));
    CHECK(direct_write_forbidden_at_path(root, {"subtree_read_only_inner", "value"}));
    CHECK(direct_write_forbidden_at_path(root,
                                         {"subtree_read_only_inner", "read_only_value"}));

    CHECK(direct_write_forbidden_at_path(root, {"fully_read_only_inner"}));
    CHECK(direct_write_forbidden_at_path(root, {"fully_read_only_inner", "value"}));
}
