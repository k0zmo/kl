#include "kl/serialization.hpp"
#include "kl/serialization_attributes.hpp"
#include "kl/json.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/yaml.hpp"

#include "input/typedefs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/node/node.h>

#include <chrono>
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

namespace {

struct serialization_record
{
    int i;
    std::string s;
    std::vector<int> values;
};

KL_REFLECT_STRUCT(serialization_record, i, s, values)

struct serialization_attributes_record
{
    int keep{};
    int skip_se{};
    int skip_de{};
    int skip_both{};
    int skip_both_manually{};
};

KL_REFLECT_STRUCT(serialization_attributes_record, keep,
                  (skip_se, kl::serialization::skip_serialization),
                  (skip_de, kl::serialization::skip_deserialization),
                  (skip_both, kl::serialization::skip),
                  (skip_both_manually, kl::serialization::skip_serialization,
                   kl::serialization::skip_deserialization))

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
