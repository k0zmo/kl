#include "kl/ctti.hpp"
#include "kl/serialization.hpp"
#include "kl/serialization_attributes.hpp"
#include "kl/json.hpp"
#include "kl/reflect_struct.hpp"
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

    template <typename Value>
    static void deserialize(std::chrono::seconds& out, const Value& value)
    {
        out = std::chrono::seconds{serialization::deserialize<long long>(value)};
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

TEST_CASE("serialization - json and yaml backends coexist")
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

        auto json_out = kl::serialization::deserialize<serialization_record>(json_value);
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

        auto yaml_out = kl::serialization::deserialize<serialization_record>(yaml_value);
        CHECK(yaml_out.i == 7);
        CHECK(yaml_out.s == "text");
        CHECK(yaml_out.values == std::vector<int>{1, 2, 3});
    }
}

TEST_CASE("serialization - skip field attributes")
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

TEST_CASE("serialization - null field attributes")
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

TEST_CASE("serialization - field aliases")
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

TEST_CASE("serialization - renamed fields")
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

TEST_CASE("serialization - generic user ADL works for json and yaml")
{
    generic_adl_value value{42};

    auto json_value = kl::json::serialize(value);
    REQUIRE(json_value.IsInt());
    CHECK(json_value.GetInt() == 42);

    auto yaml_value = kl::yaml::serialize(value);
    CHECK(yaml_value.as<int>() == 42);
}

TEST_CASE("serialization - backend ADL wins over structural reflection")
{
    reflected_with_backend_adl value{42};

    auto json_value = kl::json::serialize(value);
    REQUIRE(json_value.IsString());
    CHECK(json_value.GetString() == std::string{"json-backend-adl"});

    auto yaml_value = kl::yaml::serialize(value);
    REQUIRE(yaml_value.IsScalar());
    CHECK(yaml_value.as<std::string>() == "yaml-backend-adl");
}

TEST_CASE("serialization - chrono duration serializer works with json and yaml")
{
    using namespace std::chrono;

    chrono_test serialized{2, seconds{10}, {seconds{11}, seconds{12}}};

    auto json_value = kl::json::serialize(serialized);
    auto json_out = kl::json::deserialize<chrono_test>(json_value);
    CHECK(json_out.t == 2);
    CHECK(json_out.sec == seconds{10});
    CHECK(json_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    json_out = kl::serialization::deserialize<chrono_test>(json_value);
    CHECK(json_out.t == 2);
    CHECK(json_out.sec == seconds{10});
    CHECK(json_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    auto yaml_value = kl::yaml::serialize(serialized);
    auto yaml_out = kl::yaml::deserialize<chrono_test>(yaml_value);
    CHECK(yaml_out.t == 2);
    CHECK(yaml_out.sec == seconds{10});
    CHECK(yaml_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    yaml_out = kl::serialization::deserialize<chrono_test>(yaml_value);
    CHECK(yaml_out.t == 2);
    CHECK(yaml_out.sec == seconds{10});
    CHECK(yaml_out.secs == std::vector<seconds>{seconds{11}, seconds{12}});

    chrono_test dumped{2, seconds{10}, {seconds{6}, seconds{12}}};
    CHECK(kl::json::dump(dumped) == R"({"t":2,"sec":10,"secs":[6,12]})");
    CHECK(kl::yaml::dump(dumped) == "t: 2\nsec: 10\nsecs:\n  - 6\n  - 12");
}
