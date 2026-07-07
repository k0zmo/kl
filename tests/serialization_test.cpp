#include "kl/ctti.hpp"
#include "kl/json.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/serialization.hpp"
#include "kl/serialization_attributes.hpp"
#include "kl/serialization_error.hpp"
#include "kl/yaml.hpp"

#include "input/typedefs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/type.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace kl::serialization {

template <>
struct serializer<std::chrono::seconds>
{
    template <typename Context>
    static auto serialize(const std::chrono::seconds& t, Context& ctx)
    {
        return serialization::serialize(t.count(), ctx);
    }

    template <typename Value, typename Context>
    static void deserialize(std::chrono::seconds& out, const Value& value, Context& ctx)
    {
        out = std::chrono::seconds{serialization::deserialize<long long>(value, ctx)};
    }

    template <typename Context>
    static void dump(const std::chrono::seconds& s, Context& ctx)
    {
        serialization::dump(s.count(), ctx);
    }
};

} // namespace kl::serialization

namespace attr = kl::serialization::attributes;

namespace {

struct serialization_record
{
    int i;
    std::string s;
    std::vector<int> values;
};

KL_REFLECT_STRUCT(serialization_record, i, s,
                  (values, attr::aliases("vals", "numbers")))

struct renamed_serialization_record
{
    int id{};
    int api_token{};
    int timeout{};
};

KL_REFLECT_STRUCT(renamed_serialization_record,
                  id,
                  (api_token, attr::rename("api-token"),
                              attr::aliases("api_token", "token")),
                  (timeout, attr::rename("request-timeout")))

struct serialization_attributes_record
{
    int keep{};
    int skip_se{};
    int skip_de{};
    int skip_both{};
    int skip_both_manually{};
};

KL_REFLECT_STRUCT(serialization_attributes_record, keep,
                  (skip_se, attr::skip_serialization),
                  (skip_de, attr::skip_deserialization),
                  (skip_both, attr::skip),
                  (skip_both_manually, attr::skip_serialization, attr::skip_deserialization))

struct nullish_serialization_record
{
    std::optional<int> context_null;
    std::optional<int> skipped_null;
    std::optional<int> emitted_null;
    std::optional<int> present;
};

KL_REFLECT_STRUCT(nullish_serialization_record,
                  context_null,
                  (skipped_null, attr::skip_if_null),
                  (emitted_null, attr::emit_null),
                  present)

struct empty_serialization_record
{
    std::string context_empty;
    std::string skipped_empty;
    std::string present_string;
    std::vector<int> skipped_empty_vector;
    std::vector<int> present_vector;
};

KL_REFLECT_STRUCT(empty_serialization_record,
                  context_empty,
                  (skipped_empty, attr::skip_if_empty),
                  present_string,
                  (skipped_empty_vector, attr::skip_if_empty),
                  present_vector)

struct custom_empty_value
{
    bool empty{};
};

struct custom_empty_serialization_record
{
    custom_empty_value skipped_custom;
    custom_empty_value present_custom;
};

KL_REFLECT_STRUCT(custom_empty_serialization_record,
                  (skipped_custom, attr::skip_if_empty),
                  (present_custom, attr::skip_if_empty))

struct flattened_inner_record
{
    int b{};
    std::optional<int> skipped_null;
};

KL_REFLECT_STRUCT(flattened_inner_record,
                  b,
                  (skipped_null, attr::skip_if_null))

struct flattened_outer_record
{
    int a{};
    flattened_inner_record inner;
    int c{};
};

KL_REFLECT_STRUCT(flattened_outer_record,
                  a,
                  (inner, attr::flatten),
                  c)

struct json_extra_fields_record
{
    int a{};
    std::map<std::string, kl::json::view> extra;
};

KL_REFLECT_STRUCT(json_extra_fields_record,
                  a,
                  (extra, attr::extra_fields))

struct yaml_extra_fields_record
{
    int a{};
    std::map<std::string, kl::yaml::view> extra;
};

KL_REFLECT_STRUCT(yaml_extra_fields_record,
                  a,
                  (extra, attr::extra_fields))

struct int_extra_fields_record
{
    int a{};
    std::map<std::string, int> extra;
};

KL_REFLECT_STRUCT(int_extra_fields_record,
                  a,
                  (extra, attr::extra_fields))

struct nested_extra_fields_record
{
    std::map<std::string, int> extra;
};

KL_REFLECT_STRUCT(nested_extra_fields_record,
                  (extra, attr::extra_fields))

struct duplicate_extra_fields_record
{
    int a{};
    std::map<std::string, int> extra;
    nested_extra_fields_record nested;
};

KL_REFLECT_STRUCT(duplicate_extra_fields_record,
                  a,
                  (extra, attr::extra_fields),
                  (nested, attr::flatten))

struct explicit_string_key
{
    std::string value;

    explicit operator std::string() const { return value; }
};

struct directional_validation_record
{
    int a{};
    int skipped{};
    int write_only{};
    int read_only{};
};

KL_REFLECT_STRUCT(directional_validation_record,
                  a,
                  (skipped, attr::rename("a"), attr::skip),
                  (write_only, attr::skip_deserialization, attr::aliases("a")),
                  (read_only, attr::skip_serialization, attr::aliases("ro")))

struct defaulted_serialization_record
{
    int id{};
    int count{};
    std::string label;
    int renamed{};
    int aliased{};
};

KL_REFLECT_STRUCT(defaulted_serialization_record,
                  id,
                  (count, attr::default_value(42)),
                  (label, attr::default_value("default-label")),
                  (renamed, attr::rename("wire-renamed"), attr::default_value(7)),
                  (aliased, attr::aliases("legacy-aliased"), attr::default_value(9)))

struct allow_missing_serialization_record
{
    int id{};
    int count{11};
    std::string label{"kept"};
    int default_then_allowed{};
    int allowed_then_default{};
};

KL_REFLECT_STRUCT(allow_missing_serialization_record,
                  id,
                  (count, attr::allow_missing),
                  (label, attr::allow_missing),
                  (default_then_allowed, attr::default_value(31), attr::allow_missing),
                  (allowed_then_default, attr::allow_missing, attr::default_value(32)))

struct ranged_serialization_record
{
    int port{};
    double ratio{};
    std::uint8_t ascii{};
};

KL_REFLECT_STRUCT(ranged_serialization_record,
                  (port, attr::range(1, 65535)),
                  (ratio, attr::range(0.0, 1.0)),
                  (ascii, attr::range(0, 127)))

struct half_ranged_serialization_record
{
    int non_negative{};
    int at_most_ten{};
};

KL_REFLECT_STRUCT(half_ranged_serialization_record,
                  (non_negative, attr::greater_equal(0)),
                  (at_most_ten, attr::less_equal(10)))

struct custom_validated_serialization_record
{
    std::string name;
    int even{};
};

KL_REFLECT_STRUCT(custom_validated_serialization_record,
                  (name, attr::validate([](const std::string& value) {
                      if (value.empty())
                          throw kl::serialization::deserialize_error{"name must not be empty"};
                  })),
                  (even, attr::validate([](int value) {
                      if (value % 2 != 0)
                          throw kl::serialization::deserialize_error{"value must be even"};
                  })))

struct generic_adl_value
{
    int value;
};

template <typename Tag, typename Context>
auto serialize_adl(Tag, const generic_adl_value& value, Context& ctx)
{
    return kl::serialization::serialize(value.value, ctx);
}

struct reflected_with_backend_adl
{
    int reflected;
};

KL_REFLECT_STRUCT(reflected_with_backend_adl, reflected)

template <typename Context>
rapidjson::Value serialize_adl(kl::json::tree_tag, const reflected_with_backend_adl&, Context& ctx)
{
    return kl::json::serialize("json-backend-adl", ctx);
}

template <typename Context>
YAML::Node serialize_adl(kl::yaml::tree_tag, const reflected_with_backend_adl&, Context& ctx)
{
    return kl::yaml::serialize("yaml-backend-adl", ctx);
}

} // namespace

namespace kl::serialization {

template <>
struct empty_traits<custom_empty_value>
{
    static bool is_empty_value(const custom_empty_value& value)
    {
        return value.empty;
    }
};

template <>
struct serializer<custom_empty_value>
{
    template <typename Context>
    static auto serialize(const custom_empty_value& value, Context& ctx)
    {
        return serialization::serialize(value.empty, ctx);
    }

    template <typename Value, typename Context>
    static void deserialize(custom_empty_value& out, const Value& value, Context& ctx)
    {
        out.empty = serialization::deserialize<bool>(value, ctx);
    }

    template <typename Context>
    static void dump(const custom_empty_value& value, Context& ctx)
    {
        serialization::dump(value.empty, ctx);
    }
};

template <typename T>
using serialization_validator = kl::serialization::detail::serialize_field_names_validator<T>;
template <typename T>
using deserialization_validator = kl::serialization::detail::deserialize_field_names_validator<T>;

struct rename_collision_record
{
    int a{};
    int b{};
};
KL_REFLECT_STRUCT(rename_collision_record, a, (b, attr::rename("a")))
static_assert(!serialization_validator<rename_collision_record>::validate());
static_assert(!deserialization_validator<rename_collision_record>::validate());

struct flattened_renamed_collision_record
{
    int a{};
    int b{};
};
KL_REFLECT_STRUCT(flattened_renamed_collision_record, a, (b, attr::rename("c")))

struct rename_plus_flattened_collision_record
{
    int c{};
    flattened_renamed_collision_record inner{};
};
KL_REFLECT_STRUCT(rename_plus_flattened_collision_record, c, (inner, attr::flatten))
static_assert(!serialization_validator<rename_plus_flattened_collision_record>::validate());
static_assert(!deserialization_validator<rename_plus_flattened_collision_record>::validate());

struct flattened_collision_inner_record
{
    int a{};
};
KL_REFLECT_STRUCT(flattened_collision_inner_record, a)
static_assert(serialization_validator<flattened_collision_inner_record>::validate());
static_assert(deserialization_validator<flattened_collision_inner_record>::validate());

struct flattened_outer_collision_record
{
    int a{};
    flattened_collision_inner_record inner;
};
KL_REFLECT_STRUCT(flattened_outer_collision_record,
                  a,
                  (inner, attr::flatten))
static_assert(!serialization_validator<flattened_outer_collision_record>::validate());
static_assert(!deserialization_validator<flattened_outer_collision_record>::validate());

struct flattened_sibling_collision_record
{
    flattened_collision_inner_record left;
    flattened_collision_inner_record right;
};
KL_REFLECT_STRUCT(flattened_sibling_collision_record,
                  (left, attr::flatten),
                  (right, attr::flatten))
static_assert(!serialization_validator<flattened_sibling_collision_record>::validate());
static_assert(!deserialization_validator<flattened_sibling_collision_record>::validate());

struct flattened_alias_collision_inner_record
{
    int value{};
};
KL_REFLECT_STRUCT(flattened_alias_collision_inner_record,
                  (value, attr::aliases("a")))
static_assert(serialization_validator<flattened_alias_collision_inner_record>::validate());
static_assert(deserialization_validator<flattened_alias_collision_inner_record>::validate());

struct flattened_alias_collision_outer_record
{
    int a{};
    flattened_alias_collision_inner_record inner;
};
KL_REFLECT_STRUCT(flattened_alias_collision_outer_record,
                  a,
                  (inner, attr::flatten))
static_assert(serialization_validator<flattened_alias_collision_outer_record>::validate());
static_assert(!deserialization_validator<flattened_alias_collision_outer_record>::validate());

} // namespace kl::serialization

namespace {

struct non_literal_default_value_record
{
    int a{};
    std::string s{};
};
KL_REFLECT_STRUCT(non_literal_default_value_record,
                  a,
                  (s, attr::default_value(std::string{"x"})))

struct extra_fields_with_non_literal_default_value_record
{
    int a{};
    std::string s{};
    std::map<std::string, int> extra;
};
KL_REFLECT_STRUCT(extra_fields_with_non_literal_default_value_record,
                  a,
                  (s, attr::default_value(std::string{"x"})),
                  (extra, attr::extra_fields))

struct flattened_non_literal_default_value_record
{
    int b{};
    std::string s{};
};
KL_REFLECT_STRUCT(flattened_non_literal_default_value_record,
                  b,
                  (s, attr::default_value(std::string{"x"})))

struct flatten_with_non_literal_default_value_record
{
    int a{};
    flattened_non_literal_default_value_record inner;
};
KL_REFLECT_STRUCT(flatten_with_non_literal_default_value_record,
                  a,
                  (inner, attr::flatten))

} // namespace

TEST_CASE("serialization - json and yaml backends coexist", "[serialization]")
{
    serialization_record record{7, "text", {1, 2, 3}};

    SECTION("use backend specific namespace")
    {
        CHECK(kl::json::serialize(1).IsInt());
        CHECK(kl::json::dump(std::vector{"x", "q", "z"}) == R"(["x","q","z"])");

        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        CHECK(json_value["i"].GetInt() == 7);
        CHECK(json_value["s"].GetString() == std::string{"text"});
        REQUIRE(json_value["values"].IsArray());
        CHECK(json_value["values"].Size() == 3);

        auto json_out = kl::json::deserialize<serialization_record>(json_value);
        CHECK(json_out.i == 7);
        CHECK(json_out.s == "text");
        CHECK(json_out.values == std::vector<int>{1, 2, 3});

        CHECK(kl::yaml::serialize(1).as<int>() == 1);
        CHECK(kl::yaml::dump(std::vector{"x", "q", "z"}) == "- x\n- q\n- z");

        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        CHECK(yaml_value["i"].as<int>() == 7);
        CHECK(yaml_value["s"].as<std::string>() == "text");
        REQUIRE(yaml_value["values"].IsSequence());
        CHECK(yaml_value["values"].size() == 3);

        auto yaml_out = kl::yaml::deserialize<serialization_record>(yaml_value);
        CHECK(yaml_out.i == 7);
        CHECK(yaml_out.s == "text");
        CHECK(yaml_out.values == std::vector<int>{1, 2, 3});
    }

    SECTION("use serialization namespace")
    {
        using namespace rapidjson;

        StringBuffer sb;
        Writer<StringBuffer> writer{sb};
        kl::json::dump_context<Writer<StringBuffer>> json_dump_ctx{writer};
        kl::serialization::dump(std::vector{"x", "q", "z"}, json_dump_ctx);
        std::string res = sb.GetString();
        CHECK(res == R"(["x","q","z"])");

        kl::json::owning_serialize_context json_ctx;
        auto json_value = kl::serialization::serialize(record, json_ctx);
        REQUIRE(json_value.IsObject());
        CHECK(json_value["i"].GetInt() == 7);
        CHECK(json_value["s"].GetString() == std::string{"text"});
        REQUIRE(json_value["values"].IsArray());
        CHECK(json_value["values"].Size() == 3);

        kl::json::deserialize_context json_deserialize_ctx;
        auto json_out =
            kl::serialization::deserialize<serialization_record>(json_value,
                                                                 json_deserialize_ctx);
        CHECK(json_out.i == 7);
        CHECK(json_out.s == "text");
        CHECK(json_out.values == std::vector<int>{1, 2, 3});

        YAML::Emitter emitter;
        kl::yaml::dump_context yaml_dump_ctx{emitter};
        kl::serialization::dump(std::vector{"x", "q", "z"}, yaml_dump_ctx);
        res = emitter.c_str();
        CHECK(res == "- x\n- q\n- z");

        kl::yaml::serialize_context yaml_ctx;
        auto yaml_value = kl::serialization::serialize(record, yaml_ctx);
        REQUIRE(yaml_value.IsMap());
        CHECK(yaml_value["i"].as<int>() == 7);
        CHECK(yaml_value["s"].as<std::string>() == "text");
        REQUIRE(yaml_value["values"].IsSequence());
        CHECK(yaml_value["values"].size() == 3);

        kl::yaml::deserialize_context yaml_deserialize_ctx;
        auto yaml_out =
            kl::serialization::deserialize<serialization_record>(yaml_value,
                                                                 yaml_deserialize_ctx);
        CHECK(yaml_out.i == 7);
        CHECK(yaml_out.s == "text");
        CHECK(yaml_out.values == std::vector<int>{1, 2, 3});
    }
}

TEST_CASE("serialization - skip field attributes", "[serialization]")
{
    serialization_attributes_record record{1, 2, 3, 4};

    SECTION("skip serialization")
    {
        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        CHECK(json_value.HasMember("keep"));
        CHECK(!json_value.HasMember("skip_se"));
        CHECK(json_value.HasMember("skip_de"));
        CHECK(!json_value.HasMember("skip_both"));
        CHECK(!json_value.HasMember("skip_both_manually"));

        CHECK(kl::json::dump(record) == R"({"keep":1,"skip_de":3})");

        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        CHECK(yaml_value["keep"].as<int>() == 1);
        CHECK(!yaml_value["skip_se"]);
        CHECK(yaml_value["skip_de"].as<int>() == 3);
        CHECK(!yaml_value["skip_both"]);
        CHECK(!yaml_value["skip_both_manually"]);

        CHECK(kl::yaml::dump(record) == "keep: 1\nskip_de: 3");
    }

    SECTION("skip deserialization")
    {
        rapidjson::Document json_value;
        json_value.Parse(R"({
            "keep": 10,
            "skip_se": 20,
            "skip_de": 30,
            "skip_both": 40,
            "skip_both_manually": 50
        })");

        serialization_attributes_record json_out{1, 2, 3, 4, 5};
        kl::json::deserialize(json_out, json_value);
        CHECK(json_out.keep == 10);
        CHECK(json_out.skip_se == 20);
        CHECK(json_out.skip_de == 3);
        CHECK(json_out.skip_both == 4);
        CHECK(json_out.skip_both_manually == 5);

        YAML::Node yaml_value;
        yaml_value["keep"] = 100;
        yaml_value["skip_se"] = 200;
        yaml_value["skip_de"] = 300;
        yaml_value["skip_both"] = 400;
        yaml_value["skip_both_manually"] = 500;

        serialization_attributes_record yaml_out{1, 2, 3, 4, 5};
        kl::yaml::deserialize(yaml_out, yaml_value);
        CHECK(yaml_out.keep == 100);
        CHECK(yaml_out.skip_se == 200);
        CHECK(yaml_out.skip_de == 3);
        CHECK(yaml_out.skip_both == 4);
        CHECK(yaml_out.skip_both_manually == 5);
    }
}

TEST_CASE("serialization - null field attributes", "[serialization]")
{
    nullish_serialization_record record{std::nullopt, std::nullopt, std::nullopt, 7};

    SECTION("json")
    {
        auto default_json = kl::json::serialize(record);
        REQUIRE(default_json.IsObject());
        CHECK(!default_json.HasMember("context_null"));
        CHECK(!default_json.HasMember("skipped_null"));
        REQUIRE(default_json.HasMember("emitted_null"));
        CHECK(default_json["emitted_null"].IsNull());
        REQUIRE(default_json.HasMember("present"));
        CHECK(default_json["present"].GetInt() == 7);

        CHECK(kl::json::dump(record) == R"({"emitted_null":null,"present":7})");

        kl::json::owning_serialize_context json_ctx{false};
        auto explicit_null_json = kl::serialization::serialize(record, json_ctx);
        REQUIRE(explicit_null_json.IsObject());
        REQUIRE(explicit_null_json.HasMember("context_null"));
        CHECK(explicit_null_json["context_null"].IsNull());
        CHECK(!explicit_null_json.HasMember("skipped_null"));
        REQUIRE(explicit_null_json.HasMember("emitted_null"));
        CHECK(explicit_null_json["emitted_null"].IsNull());
        REQUIRE(explicit_null_json.HasMember("present"));
        CHECK(explicit_null_json["present"].GetInt() == 7);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer{buffer};
        kl::json::dump_context json_dump_ctx{writer, false};
        kl::serialization::dump(record, json_dump_ctx);
        CHECK(std::string{buffer.GetString()} ==
              R"({"context_null":null,"emitted_null":null,"present":7})");
    }

    SECTION("yaml")
    {
        auto default_yaml = kl::yaml::serialize(record);
        REQUIRE(default_yaml.IsMap());
        CHECK(!default_yaml["context_null"]);
        CHECK(!default_yaml["skipped_null"]);
        REQUIRE(default_yaml["emitted_null"]);
        CHECK(default_yaml["emitted_null"].IsNull());
        REQUIRE(default_yaml["present"]);
        CHECK(default_yaml["present"].as<int>() == 7);

        CHECK(kl::yaml::dump(record) == "emitted_null: ~\npresent: 7");

        kl::yaml::serialize_context yaml_ctx{false};
        auto explicit_null_yaml = kl::serialization::serialize(record, yaml_ctx);
        REQUIRE(explicit_null_yaml.IsMap());
        REQUIRE(explicit_null_yaml["context_null"]);
        CHECK(explicit_null_yaml["context_null"].IsNull());
        CHECK(!explicit_null_yaml["skipped_null"]);
        REQUIRE(explicit_null_yaml["emitted_null"]);
        CHECK(explicit_null_yaml["emitted_null"].IsNull());
        REQUIRE(explicit_null_yaml["present"]);
        CHECK(explicit_null_yaml["present"].as<int>() == 7);

        YAML::Emitter emitter;
        kl::yaml::dump_context yaml_dump_ctx{emitter, false};
        kl::serialization::dump(record, yaml_dump_ctx);
        CHECK(std::string{emitter.c_str()} ==
              "context_null: ~\nemitted_null: ~\npresent: 7");
    }
}

TEST_CASE("serialization - empty field attributes", "[serialization]")
{
    empty_serialization_record record{"", "", "present", {}, {1, 2}};

    SECTION("json")
    {
        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        REQUIRE(json_value.HasMember("context_empty"));
        CHECK(json_value["context_empty"].GetString() == std::string{});
        CHECK(!json_value.HasMember("skipped_empty"));
        REQUIRE(json_value.HasMember("present_string"));
        CHECK(json_value["present_string"].GetString() == std::string{"present"});
        CHECK(!json_value.HasMember("skipped_empty_vector"));
        REQUIRE(json_value.HasMember("present_vector"));
        CHECK(json_value["present_vector"].Size() == 2);

        CHECK(kl::json::dump(record) ==
              R"({"context_empty":"","present_string":"present","present_vector":[1,2]})");
    }

    SECTION("yaml")
    {
        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        REQUIRE(yaml_value["context_empty"]);
        CHECK(yaml_value["context_empty"].as<std::string>() == "");
        CHECK(!yaml_value["skipped_empty"]);
        REQUIRE(yaml_value["present_string"]);
        CHECK(yaml_value["present_string"].as<std::string>() == "present");
        CHECK(!yaml_value["skipped_empty_vector"]);
        REQUIRE(yaml_value["present_vector"]);
        CHECK(yaml_value["present_vector"].size() == 2);

        CHECK(kl::yaml::dump(record) ==
              "context_empty: \"\"\npresent_string: present\npresent_vector:\n  - 1\n  - 2");
    }
}

TEST_CASE("serialization - empty traits customization point", "[serialization]")
{
    custom_empty_serialization_record record{{true}, {false}};

    SECTION("json")
    {
        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        CHECK(!json_value.HasMember("skipped_custom"));
        REQUIRE(json_value.HasMember("present_custom"));
        CHECK_FALSE(json_value["present_custom"].GetBool());

        CHECK(kl::json::dump(record) == R"({"present_custom":false})");
    }

    SECTION("yaml")
    {
        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        CHECK(!yaml_value["skipped_custom"]);
        REQUIRE(yaml_value["present_custom"]);
        CHECK_FALSE(yaml_value["present_custom"].as<bool>());

        CHECK(kl::yaml::dump(record) == "present_custom: false");
    }
}

TEST_CASE("serialization - flatten reflected field", "[serialization]")
{
    flattened_outer_record record{1, {2, std::nullopt}, 3};

    SECTION("json")
    {
        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        REQUIRE(json_value.HasMember("a"));
        CHECK(json_value["a"].GetInt() == 1);
        REQUIRE(json_value.HasMember("b"));
        CHECK(json_value["b"].GetInt() == 2);
        CHECK(!json_value.HasMember("inner"));
        CHECK(!json_value.HasMember("skipped_null"));
        REQUIRE(json_value.HasMember("c"));
        CHECK(json_value["c"].GetInt() == 3);

        CHECK(kl::json::dump(record) == R"({"a":1,"b":2,"c":3})");

        rapidjson::Document flat = R"({
            "a": 10,
            "b": 20,
            "c": 30
        })"_json;

        auto json_out = kl::json::deserialize<flattened_outer_record>(flat);
        CHECK(json_out.a == 10);
        CHECK(json_out.inner.b == 20);
        CHECK(!json_out.inner.skipped_null);
        CHECK(json_out.c == 30);
    }

    SECTION("yaml")
    {
        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        REQUIRE(yaml_value["a"]);
        CHECK(yaml_value["a"].as<int>() == 1);
        REQUIRE(yaml_value["b"]);
        CHECK(yaml_value["b"].as<int>() == 2);
        CHECK(!yaml_value["inner"]);
        CHECK(!yaml_value["skipped_null"]);
        REQUIRE(yaml_value["c"]);
        CHECK(yaml_value["c"].as<int>() == 3);

        CHECK(kl::yaml::dump(record) == "a: 1\nb: 2\nc: 3");

        auto flat = R"(
a: 10
b: 20
c: 30
)"_yaml;

        auto yaml_out = kl::yaml::deserialize<flattened_outer_record>(flat);
        CHECK(yaml_out.a == 10);
        CHECK(yaml_out.inner.b == 20);
        CHECK(!yaml_out.inner.skipped_null);
        CHECK(yaml_out.c == 30);
    }
}

TEST_CASE("serialization - extra fields attribute", "[serialization]")
{
    SECTION("json")
    {
        rapidjson::Document value = R"({
            "a": 1,
            "valueExtra": [1, 2, 3],
            "otherExtra": {"nested": true}
        })"_json;

        auto json_out = kl::json::deserialize<json_extra_fields_record>(value);
        CHECK(json_out.a == 1);
        REQUIRE(json_out.extra.size() == 2);
        REQUIRE(json_out.extra.count("valueExtra") == 1);
        REQUIRE(json_out.extra["valueExtra"]->IsArray());
        CHECK(json_out.extra["valueExtra"]->Size() == 3);
        REQUIRE(json_out.extra.count("otherExtra") == 1);
        REQUIRE(json_out.extra["otherExtra"]->IsObject());
        CHECK(json_out.extra["otherExtra"]->HasMember("nested"));

        auto serialized = kl::json::serialize(json_out);
        REQUIRE(serialized.IsObject());
        REQUIRE(serialized.HasMember("a"));
        CHECK(serialized["a"].GetInt() == 1);
        REQUIRE(serialized.HasMember("valueExtra"));
        CHECK(serialized["valueExtra"].Size() == 3);
        REQUIRE(serialized.HasMember("otherExtra"));
        CHECK(serialized["otherExtra"]["nested"].GetBool());
        CHECK(!serialized.HasMember("extra"));

        CHECK(kl::json::dump(json_out) ==
              R"({"a":1,"otherExtra":{"nested":true},"valueExtra":[1,2,3]})");

        rapidjson::Document invalid_extra = R"({
            "a": 1,
            "badExtra": [1, 2, 3]
        })"_json;

        try
        {
            (void)kl::json::deserialize<int_extra_fields_record>(invalid_extra);
            FAIL("expected deserialize_error");
        }
        catch (const kl::serialization::deserialize_error& ex)
        {
            const std::string message = ex.what();
            CHECK(message.find("error when deserializing extra field badExtra") !=
                  std::string::npos);
            CHECK(message.find("error when deserializing field extra") == std::string::npos);
        }
    }

    SECTION("yaml")
    {
        auto value = R"(
a: 1
valueExtra:
  - 1
  - 2
  - 3
otherExtra:
  nested: true
)"_yaml;

        auto yaml_out = kl::yaml::deserialize<yaml_extra_fields_record>(value);
        CHECK(yaml_out.a == 1);
        REQUIRE(yaml_out.extra.size() == 2);
        REQUIRE(yaml_out.extra.count("valueExtra") == 1);
        REQUIRE(yaml_out.extra["valueExtra"]->IsSequence());
        CHECK(yaml_out.extra["valueExtra"]->size() == 3);
        REQUIRE(yaml_out.extra.count("otherExtra") == 1);
        REQUIRE(yaml_out.extra["otherExtra"]->IsMap());
        CHECK(yaml_out.extra["otherExtra"]->operator[]("nested").as<bool>());

        auto serialized = kl::yaml::serialize(yaml_out);
        REQUIRE(serialized.IsMap());
        REQUIRE(serialized["a"]);
        CHECK(serialized["a"].as<int>() == 1);
        REQUIRE(serialized["valueExtra"]);
        CHECK(serialized["valueExtra"].size() == 3);
        REQUIRE(serialized["otherExtra"]);
        CHECK(serialized["otherExtra"]["nested"].as<bool>());
        CHECK(!serialized["extra"]);

        CHECK(kl::yaml::dump(yaml_out) ==
              "a: 1\notherExtra:\n  nested: true\nvalueExtra:\n  - 1\n  - 2\n  - 3");

        auto invalid_extra = R"(
a: 1
badExtra:
  - 1
  - 2
)"_yaml;

        try
        {
            (void)kl::yaml::deserialize<int_extra_fields_record>(invalid_extra);
            FAIL("expected deserialize_error");
        }
        catch (const kl::serialization::deserialize_error& ex)
        {
            const std::string message = ex.what();
            CHECK(message.find("error when deserializing extra field badExtra") !=
                  std::string::npos);
            CHECK(message.find("error when deserializing field extra") == std::string::npos);
        }
    }
}

TEST_CASE("serialization - extra fields reject reserved keys", "[serialization]")
{
    SECTION("transparent string lookup")
    {
        int_extra_fields_record record{1, {{"a", 2}}};

        CHECK_THROWS_WITH(kl::json::serialize(record),
                          "serialization extra_fields key collides with reflected field: a");
        CHECK_THROWS_WITH(kl::json::dump(record),
                          "serialization extra_fields key collides with reflected field: a");
        CHECK_THROWS_WITH(kl::yaml::serialize(record),
                          "serialization extra_fields key collides with reflected field: a");
        CHECK_THROWS_WITH(kl::yaml::dump(record),
                          "serialization extra_fields key collides with reflected field: a");
    }

    SECTION("explicit string construction fallback")
    {
        kl::serialization::detail::string_set reserved_names{"a"};
        explicit_string_key key{"a"};

        CHECK_THROWS_WITH(kl::serialization::detail::check_extra_field_key(reserved_names, key),
                          "serialization extra_fields key collides with reflected field: a");
    }
}

TEST_CASE("serialization - extra fields rejects multiple fields", "[serialization]")
{
    duplicate_extra_fields_record record{1, {{"outer", 2}}, {{{"inner", 3}}}};

    CHECK_THROWS_WITH(kl::json::serialize(record),
                      "serialization only one extra_fields field is supported");
    CHECK_THROWS_WITH(kl::json::dump(record),
                      "serialization only one extra_fields field is supported");
    CHECK_THROWS_WITH(kl::yaml::serialize(record),
                      "serialization only one extra_fields field is supported");
    CHECK_THROWS_WITH(kl::yaml::dump(record),
                      "serialization only one extra_fields field is supported");

    rapidjson::Document json_value = R"({"a": 1, "unknown": 2})"_json;
    CHECK_THROWS_WITH(kl::json::deserialize<duplicate_extra_fields_record>(json_value),
                      "serialization only one extra_fields field is supported");

    auto yaml_value = R"(
a: 1
unknown: 2
)"_yaml;
    CHECK_THROWS_WITH(kl::yaml::deserialize<duplicate_extra_fields_record>(yaml_value),
                      "serialization only one extra_fields field is supported");
}

TEST_CASE("serialization - reflected name validation respects direction",
          "[serialization]")
{
    directional_validation_record record{1, 2, 3, 4};

    auto json_value = kl::json::serialize(record);
    REQUIRE(json_value.IsObject());
    CHECK(json_value.HasMember("a"));
    CHECK(json_value.HasMember("write_only"));
    CHECK(!json_value.HasMember("skipped"));
    CHECK(!json_value.HasMember("read_only"));

    rapidjson::Document json_input = R"({
        "a": 10,
        "ro": 40
    })"_json;
    auto json_out = kl::json::deserialize<directional_validation_record>(json_input);
    CHECK(json_out.a == 10);
    CHECK(json_out.read_only == 40);

    auto yaml_value = kl::yaml::serialize(record);
    REQUIRE(yaml_value.IsMap());
    CHECK(yaml_value["a"].as<int>() == 1);
    CHECK(yaml_value["write_only"].as<int>() == 3);
    CHECK(!yaml_value["skipped"]);
    CHECK(!yaml_value["read_only"]);

    auto yaml_input = R"(
a: 10
ro: 40
)"_yaml;
    auto yaml_out = kl::yaml::deserialize<directional_validation_record>(yaml_input);
    CHECK(yaml_out.a == 10);
    CHECK(yaml_out.read_only == 40);
}

TEST_CASE("serialization - default value attribute", "[serialization]")
{
    SECTION("json")
    {
        rapidjson::Document missing = R"({
            "id": 1
        })"_json;

        auto json_out = kl::json::deserialize<defaulted_serialization_record>(missing);
        CHECK(json_out.id == 1);
        CHECK(json_out.count == 42);
        CHECK(json_out.label == "default-label");
        CHECK(json_out.renamed == 7);
        CHECK(json_out.aliased == 9);

        rapidjson::Document non_literal_missing = R"({
            "a": 1
        })"_json;
        auto non_literal_json =
            kl::json::deserialize<non_literal_default_value_record>(non_literal_missing);
        CHECK(non_literal_json.a == 1);
        CHECK(non_literal_json.s == "x");

        rapidjson::Document extra_non_literal_missing = R"({
            "a": 2,
            "bonus": 3
        })"_json;
        auto extra_non_literal_json =
            kl::json::deserialize<extra_fields_with_non_literal_default_value_record>(
                extra_non_literal_missing);
        CHECK(extra_non_literal_json.a == 2);
        CHECK(extra_non_literal_json.s == "x");
        CHECK(extra_non_literal_json.extra == std::map<std::string, int>{{"bonus", 3}});

        auto serialized_extra_non_literal = kl::json::serialize(extra_non_literal_json);
        REQUIRE(serialized_extra_non_literal.IsObject());
        CHECK(serialized_extra_non_literal["a"].GetInt() == 2);
        CHECK(serialized_extra_non_literal["s"].GetString() == std::string{"x"});
        CHECK(serialized_extra_non_literal["bonus"].GetInt() == 3);

        rapidjson::Document flattened_non_literal_missing = R"({
            "a": 4,
            "b": 5
        })"_json;
        auto flattened_non_literal_json =
            kl::json::deserialize<flatten_with_non_literal_default_value_record>(
                flattened_non_literal_missing);
        CHECK(flattened_non_literal_json.a == 4);
        CHECK(flattened_non_literal_json.inner.b == 5);
        CHECK(flattened_non_literal_json.inner.s == "x");

        auto serialized_flattened_non_literal = kl::json::serialize(flattened_non_literal_json);
        REQUIRE(serialized_flattened_non_literal.IsObject());
        CHECK(serialized_flattened_non_literal["a"].GetInt() == 4);
        CHECK(serialized_flattened_non_literal["b"].GetInt() == 5);
        CHECK(serialized_flattened_non_literal["s"].GetString() == std::string{"x"});

        rapidjson::Document nulls = R"({
            "id": 2,
            "count": null,
            "label": null,
            "wire-renamed": null,
            "aliased": null
        })"_json;

        json_out = kl::json::deserialize<defaulted_serialization_record>(nulls);
        CHECK(json_out.id == 2);
        CHECK(json_out.count == 42);
        CHECK(json_out.label == "default-label");
        CHECK(json_out.renamed == 7);
        CHECK(json_out.aliased == 9);

        rapidjson::Document present = R"({
            "id": 3,
            "count": 100,
            "label": "present-label",
            "wire-renamed": 101,
            "legacy-aliased": 102
        })"_json;

        json_out = kl::json::deserialize<defaulted_serialization_record>(present);
        CHECK(json_out.id == 3);
        CHECK(json_out.count == 100);
        CHECK(json_out.label == "present-label");
        CHECK(json_out.renamed == 101);
        CHECK(json_out.aliased == 102);
    }

    SECTION("yaml")
    {
        YAML::Node missing;
        missing["id"] = 1;

        auto yaml_out = kl::yaml::deserialize<defaulted_serialization_record>(missing);
        CHECK(yaml_out.id == 1);
        CHECK(yaml_out.count == 42);
        CHECK(yaml_out.label == "default-label");
        CHECK(yaml_out.renamed == 7);
        CHECK(yaml_out.aliased == 9);

        YAML::Node nulls;
        nulls["id"] = 2;
        nulls["count"] = YAML::Node{};
        nulls["label"] = YAML::Node{};
        nulls["wire-renamed"] = YAML::Node{};
        nulls["aliased"] = YAML::Node{};

        yaml_out = kl::yaml::deserialize<defaulted_serialization_record>(nulls);
        CHECK(yaml_out.id == 2);
        CHECK(yaml_out.count == 42);
        CHECK(yaml_out.label == "default-label");
        CHECK(yaml_out.renamed == 7);
        CHECK(yaml_out.aliased == 9);

        YAML::Node present;
        present["id"] = 3;
        present["count"] = 100;
        present["label"] = "present-label";
        present["wire-renamed"] = 101;
        present["legacy-aliased"] = 102;

        yaml_out = kl::yaml::deserialize<defaulted_serialization_record>(present);
        CHECK(yaml_out.id == 3);
        CHECK(yaml_out.count == 100);
        CHECK(yaml_out.label == "present-label");
        CHECK(yaml_out.renamed == 101);
        CHECK(yaml_out.aliased == 102);
    }
}

TEST_CASE("serialization - allow missing attribute", "[serialization]")
{
    SECTION("json")
    {
        rapidjson::Document missing = R"({
            "id": 1
        })"_json;

        auto json_out = kl::json::deserialize<allow_missing_serialization_record>(missing);
        CHECK(json_out.id == 1);
        CHECK(json_out.count == 11);
        CHECK(json_out.label == "kept");
        CHECK(json_out.default_then_allowed == 31);
        CHECK(json_out.allowed_then_default == 32);

        rapidjson::Document present = R"({
            "id": 2,
            "count": 21,
            "label": "present",
            "default_then_allowed": 22,
            "allowed_then_default": 23
        })"_json;

        json_out = kl::json::deserialize<allow_missing_serialization_record>(present);
        CHECK(json_out.id == 2);
        CHECK(json_out.count == 21);
        CHECK(json_out.label == "present");
        CHECK(json_out.default_then_allowed == 22);
        CHECK(json_out.allowed_then_default == 23);

        rapidjson::Document null = R"({
            "id": 3,
            "count": null
        })"_json;

        CHECK_THROWS_AS(kl::json::deserialize<allow_missing_serialization_record>(null),
                        kl::serialization::deserialize_error);
    }

    SECTION("yaml")
    {
        auto missing = R"(
id: 1
)"_yaml;

        auto yaml_out = kl::yaml::deserialize<allow_missing_serialization_record>(missing);
        CHECK(yaml_out.id == 1);
        CHECK(yaml_out.count == 11);
        CHECK(yaml_out.label == "kept");
        CHECK(yaml_out.default_then_allowed == 31);
        CHECK(yaml_out.allowed_then_default == 32);

        auto present = R"(
id: 2
count: 21
label: present
default_then_allowed: 22
allowed_then_default: 23
)"_yaml;

        yaml_out = kl::yaml::deserialize<allow_missing_serialization_record>(present);
        CHECK(yaml_out.id == 2);
        CHECK(yaml_out.count == 21);
        CHECK(yaml_out.label == "present");
        CHECK(yaml_out.default_then_allowed == 22);
        CHECK(yaml_out.allowed_then_default == 23);

        auto null = R"(
id: 3
count: ~
)"_yaml;

        CHECK_THROWS_AS(kl::yaml::deserialize<allow_missing_serialization_record>(null),
                        kl::serialization::deserialize_error);
    }
}

TEST_CASE("serialization - range attribute", "[serialization]")
{
    SECTION("json")
    {
        rapidjson::Document valid = R"({
            "port": 1,
            "ratio": 1.0,
            "ascii": 65
        })"_json;

        auto json_out = kl::json::deserialize<ranged_serialization_record>(valid);
        CHECK(json_out.port == 1);
        CHECK(json_out.ratio == 1.0);
        CHECK(json_out.ascii == 65);

        rapidjson::Document port_too_low = R"({
            "port": 0,
            "ratio": 0.5,
            "ascii": 65
        })"_json;

        CHECK_THROWS_WITH(kl::json::deserialize<ranged_serialization_record>(port_too_low),
                          "value is outside allowed range [1, 65535]\n"
                          "error when deserializing field port\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        rapidjson::Document ratio_too_high = R"({
            "port": 1234,
            "ratio": 1.5,
            "ascii": 65
        })"_json;

        CHECK_THROWS_WITH(kl::json::deserialize<ranged_serialization_record>(ratio_too_high),
                          "value is outside allowed range [0.000000, 1.000000]\n"
                          "error when deserializing field ratio\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        rapidjson::Document ascii_to_high = R"({
            "port": 1234,
            "ratio": 1.0,
            "ascii": 150
        })"_json;

        CHECK_THROWS_WITH(kl::json::deserialize<ranged_serialization_record>(ascii_to_high),
                          "value is outside allowed range [0, 127]\n"
                          "error when deserializing field ascii\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        rapidjson::Document sequence_port_too_low = R"([0, 0.5, 70])"_json;
        CHECK_THROWS_WITH(kl::json::deserialize<ranged_serialization_record>(
                              sequence_port_too_low),
                          "value is outside allowed range [1, 65535]\n"
                          "error when deserializing element 0\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        auto serialized = kl::json::serialize(ranged_serialization_record{70000, 2.0, 250});
        REQUIRE(serialized.IsObject());
        CHECK(serialized["port"].GetInt() == 70000);
        CHECK(serialized["ratio"].GetDouble() == 2.0);
        CHECK(serialized["ascii"].GetDouble() == 250);

        rapidjson::Document half_valid = R"({
            "non_negative": 0,
            "at_most_ten": 10
        })"_json;
        auto half_json_out =
            kl::json::deserialize<half_ranged_serialization_record>(half_valid);
        CHECK(half_json_out.non_negative == 0);
        CHECK(half_json_out.at_most_ten == 10);

        rapidjson::Document negative = R"({
            "non_negative": -1,
            "at_most_ten": 10
        })"_json;
        CHECK_THROWS_WITH(kl::json::deserialize<half_ranged_serialization_record>(negative),
                          "value is less than allowed limit of 0\n"
                          "error when deserializing field non_negative\n"
                          "error when deserializing type " +
                              kl::ctti::name<half_ranged_serialization_record>());

        rapidjson::Document too_large = R"({
            "non_negative": 0,
            "at_most_ten": 11
        })"_json;
        CHECK_THROWS_WITH(kl::json::deserialize<half_ranged_serialization_record>(too_large),
                          "value is greater than allowed limit of 10\n"
                          "error when deserializing field at_most_ten\n"
                          "error when deserializing type " +
                              kl::ctti::name<half_ranged_serialization_record>());
    }

    SECTION("yaml")
    {
        auto valid = R"(
port: 65535
ratio: 0.0
ascii: 65
)"_yaml;

        auto yaml_out = kl::yaml::deserialize<ranged_serialization_record>(valid);
        CHECK(yaml_out.port == 65535);
        CHECK(yaml_out.ratio == 0.0);

        auto port_too_low = R"(
port: 0
ratio: 0.5
ascii: 65
)"_yaml;

        CHECK_THROWS_WITH(kl::yaml::deserialize<ranged_serialization_record>(port_too_low),
                          "value is outside allowed range [1, 65535]\n"
                          "error when deserializing field port\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        auto ratio_too_high = R"(
port: 1234
ratio: 1.5
ascii: 65
)"_yaml;

        CHECK_THROWS_WITH(kl::yaml::deserialize<ranged_serialization_record>(ratio_too_high),
                          "value is outside allowed range [0.000000, 1.000000]\n"
                          "error when deserializing field ratio\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        auto ascii_too_high = R"(
port: 1234
ratio: 0.5
ascii: 150
)"_yaml;

        CHECK_THROWS_WITH(kl::yaml::deserialize<ranged_serialization_record>(ascii_too_high),
                          "value is outside allowed range [0, 127]\n"
                          "error when deserializing field ascii\n"
                          "error when deserializing type " +
                              kl::ctti::name<ranged_serialization_record>());

        auto serialized = kl::yaml::serialize(ranged_serialization_record{70000, 2.0, 250});
        REQUIRE(serialized.IsMap());
        CHECK(serialized["port"].as<int>() == 70000);
        CHECK(serialized["ratio"].as<double>() == 2.0);
        CHECK(serialized["ascii"].as<std::uint8_t>() == 250);

        auto half_valid = R"(
non_negative: 0
at_most_ten: 10
)"_yaml;
        auto half_yaml_out =
            kl::yaml::deserialize<half_ranged_serialization_record>(half_valid);
        CHECK(half_yaml_out.non_negative == 0);
        CHECK(half_yaml_out.at_most_ten == 10);

        auto negative = R"(
non_negative: -1
at_most_ten: 10
)"_yaml;
        CHECK_THROWS_WITH(kl::yaml::deserialize<half_ranged_serialization_record>(negative),
                          "value is less than allowed limit of 0\n"
                          "error when deserializing field non_negative\n"
                          "error when deserializing type " +
                              kl::ctti::name<half_ranged_serialization_record>());

        auto too_large = R"(
non_negative: 0
at_most_ten: 11
)"_yaml;
        CHECK_THROWS_WITH(kl::yaml::deserialize<half_ranged_serialization_record>(too_large),
                          "value is greater than allowed limit of 10\n"
                          "error when deserializing field at_most_ten\n"
                          "error when deserializing type " +
                              kl::ctti::name<half_ranged_serialization_record>());
    }
}

TEST_CASE("serialization - custom validate attribute", "[serialization]")
{
    SECTION("json")
    {
        rapidjson::Document valid = R"({
            "name": "abc",
            "even": 4
        })"_json;

        auto json_out = kl::json::deserialize<custom_validated_serialization_record>(valid);
        CHECK(json_out.name == "abc");
        CHECK(json_out.even == 4);

        rapidjson::Document empty_name = R"({
            "name": "",
            "even": 4
        })"_json;
        CHECK_THROWS_WITH(
            kl::json::deserialize<custom_validated_serialization_record>(empty_name),
            "name must not be empty\n"
            "error when deserializing field name\n"
            "error when deserializing type " +
                kl::ctti::name<custom_validated_serialization_record>());

        rapidjson::Document odd = R"({
            "name": "abc",
            "even": 3
        })"_json;
        CHECK_THROWS_WITH(kl::json::deserialize<custom_validated_serialization_record>(odd),
                          "value must be even\n"
                          "error when deserializing field even\n"
                          "error when deserializing type " +
                              kl::ctti::name<custom_validated_serialization_record>());

        rapidjson::Document sequence_odd = R"(["abc", 3])"_json;
        CHECK_THROWS_WITH(
            kl::json::deserialize<custom_validated_serialization_record>(sequence_odd),
            "value must be even\n"
            "error when deserializing element 1\n"
            "error when deserializing type " +
                kl::ctti::name<custom_validated_serialization_record>());

        auto serialized = kl::json::serialize(custom_validated_serialization_record{"", 3});
        REQUIRE(serialized.IsObject());
        CHECK(serialized["name"].GetString() == std::string{});
        CHECK(serialized["even"].GetInt() == 3);
    }

    SECTION("yaml")
    {
        auto valid = R"(
name: abc
even: 4
)"_yaml;

        auto yaml_out = kl::yaml::deserialize<custom_validated_serialization_record>(valid);
        CHECK(yaml_out.name == "abc");
        CHECK(yaml_out.even == 4);

        auto empty_name = R"(
name: ""
even: 4
)"_yaml;
        CHECK_THROWS_WITH(
            kl::yaml::deserialize<custom_validated_serialization_record>(empty_name),
            "name must not be empty\n"
            "error when deserializing field name\n"
            "error when deserializing type " +
                kl::ctti::name<custom_validated_serialization_record>());

        auto odd = R"(
name: abc
even: 3
)"_yaml;
        CHECK_THROWS_WITH(kl::yaml::deserialize<custom_validated_serialization_record>(odd),
                          "value must be even\n"
                          "error when deserializing field even\n"
                          "error when deserializing type " +
                              kl::ctti::name<custom_validated_serialization_record>());

        auto serialized = kl::yaml::serialize(custom_validated_serialization_record{"", 3});
        REQUIRE(serialized.IsMap());
        CHECK(serialized["name"].as<std::string>() == "");
        CHECK(serialized["even"].as<int>() == 3);
    }
}

TEST_CASE("serialization - field aliases", "[serialization]")
{
    serialization_record record{7, "text", {1, 2, 3}};

    SECTION("json")
    {
        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        CHECK(json_value.HasMember("i"));
        CHECK(json_value.HasMember("s"));
        CHECK(json_value.HasMember("values"));
        CHECK(!json_value.HasMember("vals"));
        CHECK(!json_value.HasMember("numbers"));

        auto json_out = kl::json::deserialize<serialization_record>(json_value);
        CHECK(json_out.i == 7);
        CHECK(json_out.s == "text");
        CHECK(json_out.values == std::vector<int>{1, 2, 3});

        rapidjson::Document json_alias1 = R"({
            "i": 7,
            "s": "text",
            "vals": [1, 2, 3]
        })"_json;

        json_out = kl::json::deserialize<serialization_record>(json_alias1);
        CHECK(json_out.i == 7);
        CHECK(json_out.s == "text");
        CHECK(json_out.values == std::vector<int>{1, 2, 3});

        rapidjson::Document json_alias2 = R"({
            "i": 7,
            "s": "text",
            "numbers": [1, 2, 3]
        })"_json;

        json_out = kl::json::deserialize<serialization_record>(json_alias2);
        CHECK(json_out.i == 7);
        CHECK(json_out.s == "text");
        CHECK(json_out.values == std::vector<int>{1, 2, 3});

        rapidjson::Document json_alias3 = R"({
            "i": 7,
            "s": "text"
        })"_json;

        CHECK_THROWS_WITH(kl::json::deserialize<serialization_record>(json_alias3),
                          "type must be an array but is a kNullType\n"
                          "error when deserializing field values\n"
                          "error when deserializing type " +
                              kl::ctti::name<serialization_record>());
    }

    SECTION("yaml")
    {
        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        CHECK(yaml_value["i"].as<int>() == 7);
        CHECK(yaml_value["s"].as<std::string>() == "text");
        REQUIRE(yaml_value["values"].IsSequence());
        CHECK(yaml_value["values"].size() == 3);

        auto yaml_out = kl::yaml::deserialize<serialization_record>(yaml_value);
        CHECK(yaml_out.i == 7);
        CHECK(yaml_out.s == "text");
        CHECK(yaml_out.values == std::vector<int>{1, 2, 3});

        YAML::Node yaml_alias1;
        yaml_alias1["i"] = 7;
        yaml_alias1["s"] = "text";
        yaml_alias1["vals"] = YAML::Node(YAML::NodeType::Sequence);
        yaml_alias1["vals"].push_back(1);
        yaml_alias1["vals"].push_back(2);
        yaml_alias1["vals"].push_back(3);

        yaml_out = kl::yaml::deserialize<serialization_record>(yaml_alias1);
        CHECK(yaml_out.i == 7);
        CHECK(yaml_out.s == "text");
        CHECK(yaml_out.values == std::vector<int>{1, 2, 3});

        YAML::Node yaml_alias2;
        yaml_alias2["i"] = 7;
        yaml_alias2["s"] = "text";
        yaml_alias2["numbers"] = YAML::Node(YAML::NodeType::Sequence);
        yaml_alias2["numbers"].push_back(1);
        yaml_alias2["numbers"].push_back(2);
        yaml_alias2["numbers"].push_back(3);

        yaml_out = kl::yaml::deserialize<serialization_record>(yaml_alias2);
        CHECK(yaml_out.i == 7);
        CHECK(yaml_out.s == "text");
        CHECK(yaml_out.values == std::vector<int>{1, 2, 3});

        YAML::Node yaml_alias3;
        yaml_alias3["i"] = 7;
        yaml_alias3["s"] = "text";

        CHECK_THROWS_WITH(kl::yaml::deserialize<serialization_record>(yaml_alias3),
                          "type must be a sequence but is a Null\n"
                          "error when deserializing field values\n"
                          "error when deserializing type " +
                              kl::ctti::name<serialization_record>());
    }
}

TEST_CASE("serialization - renamed fields", "[serialization]")
{
    renamed_serialization_record record{1, 2, 3};

    SECTION("json")
    {
        auto json_value = kl::json::serialize(record);
        REQUIRE(json_value.IsObject());
        CHECK(json_value.HasMember("id"));
        CHECK(json_value.HasMember("api-token"));
        CHECK(json_value.HasMember("request-timeout"));
        CHECK(!json_value.HasMember("api_token"));
        CHECK(!json_value.HasMember("timeout"));
        CHECK(json_value["api-token"].GetInt() == 2);
        CHECK(json_value["request-timeout"].GetInt() == 3);

        CHECK(kl::json::dump(record) == R"({"id":1,"api-token":2,"request-timeout":3})");

        rapidjson::Document renamed = R"({
            "id": 10,
            "api-token": 20,
            "request-timeout": 30
        })"_json;

        auto json_out = kl::json::deserialize<renamed_serialization_record>(renamed);
        CHECK(json_out.id == 10);
        CHECK(json_out.api_token == 20);
        CHECK(json_out.timeout == 30);

        rapidjson::Document alias = R"({
            "id": 10,
            "token": 21,
            "request-timeout": 30
        })"_json;

        json_out = kl::json::deserialize<renamed_serialization_record>(alias);
        CHECK(json_out.id == 10);
        CHECK(json_out.api_token == 21);
        CHECK(json_out.timeout == 30);

        rapidjson::Document canonical_wins = R"({
            "id": 10,
            "api-token": 20,
            "token": 21,
            "request-timeout": 30
        })"_json;

        json_out = kl::json::deserialize<renamed_serialization_record>(canonical_wins);
        CHECK(json_out.id == 10);
        CHECK(json_out.api_token == 20);
        CHECK(json_out.timeout == 30);
    }

    SECTION("yaml")
    {
        auto yaml_value = kl::yaml::serialize(record);
        REQUIRE(yaml_value.IsMap());
        CHECK(yaml_value["id"].as<int>() == 1);
        CHECK(yaml_value["api-token"].as<int>() == 2);
        CHECK(yaml_value["request-timeout"].as<int>() == 3);
        CHECK(!yaml_value["api_token"]);
        CHECK(!yaml_value["timeout"]);

        CHECK(kl::yaml::dump(record) == "id: 1\napi-token: 2\nrequest-timeout: 3");

        YAML::Node renamed;
        renamed["id"] = 10;
        renamed["api-token"] = 20;
        renamed["request-timeout"] = 30;

        auto yaml_out = kl::yaml::deserialize<renamed_serialization_record>(renamed);
        CHECK(yaml_out.id == 10);
        CHECK(yaml_out.api_token == 20);
        CHECK(yaml_out.timeout == 30);

        YAML::Node alias;
        alias["id"] = 10;
        alias["token"] = 21;
        alias["request-timeout"] = 30;

        yaml_out = kl::yaml::deserialize<renamed_serialization_record>(alias);
        CHECK(yaml_out.id == 10);
        CHECK(yaml_out.api_token == 21);
        CHECK(yaml_out.timeout == 30);

        YAML::Node canonical_wins;
        canonical_wins["id"] = 10;
        canonical_wins["api-token"] = 20;
        canonical_wins["token"] = 21;
        canonical_wins["request-timeout"] = 30;

        yaml_out = kl::yaml::deserialize<renamed_serialization_record>(canonical_wins);
        CHECK(yaml_out.id == 10);
        CHECK(yaml_out.api_token == 20);
        CHECK(yaml_out.timeout == 30);
    }
}

TEST_CASE("serialization - generic user ADL works for json and yaml", "[serialization]")
{
    generic_adl_value value{42};

    auto json_value = kl::json::serialize(value);
    REQUIRE(json_value.IsInt());
    CHECK(json_value.GetInt() == 42);

    auto yaml_value = kl::yaml::serialize(value);
    CHECK(yaml_value.as<int>() == 42);
}

TEST_CASE("serialization - backend ADL wins over structural reflection", "[serialization]")
{
    reflected_with_backend_adl value{42};

    auto json_value = kl::json::serialize(value);
    REQUIRE(json_value.IsString());
    CHECK(json_value.GetString() == std::string{"json-backend-adl"});

    auto yaml_value = kl::yaml::serialize(value);
    REQUIRE(yaml_value.IsScalar());
    CHECK(yaml_value.as<std::string>() == "yaml-backend-adl");
}

TEST_CASE("serialization - chrono duration serializer works with json and yaml", "[serialization]")
{
    using namespace std::chrono;

    chrono_test serialized{2, seconds{10}, {seconds{11}, seconds{12}}};

    auto json_value = kl::json::serialize(serialized);
    auto json_out = kl::json::deserialize<chrono_test>(json_value);
    CHECK(json_out.t == 2);
    CHECK(json_out.sec == seconds{10});
    CHECK(json_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    kl::json::deserialize_context json_ctx;
    json_out = kl::serialization::deserialize<chrono_test>(json_value, json_ctx);
    CHECK(json_out.t == 2);
    CHECK(json_out.sec == seconds{10});
    CHECK(json_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    auto yaml_value = kl::yaml::serialize(serialized);
    auto yaml_out = kl::yaml::deserialize<chrono_test>(yaml_value);
    CHECK(yaml_out.t == 2);
    CHECK(yaml_out.sec == seconds{10});
    CHECK(yaml_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    kl::yaml::deserialize_context yaml_ctx;
    yaml_out = kl::serialization::deserialize<chrono_test>(yaml_value, yaml_ctx);
    CHECK(yaml_out.t == 2);
    CHECK(yaml_out.sec == seconds{10});
    CHECK(yaml_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    chrono_test dumped{2, seconds{10}, {seconds{6}, seconds{12}}};
    CHECK(kl::json::dump(dumped) == R"({"t":2,"sec":10,"secs":[6,12]})");
    CHECK(kl::yaml::dump(dumped) == "t: 2\nsec: 10\nsecs:\n  - 6\n  - 12");
}
