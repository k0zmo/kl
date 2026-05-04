#include "kl/json.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/yaml.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace {

struct serialization_record
{
    int i;
    std::string s;
    std::vector<int> values;
};

KL_REFLECT_STRUCT(serialization_record, i, s, values)

} // namespace

TEST_CASE("serialization - json and yaml backends coexist")
{
    CHECK(kl::json::serialize(1).IsInt());
    CHECK(kl::yaml::serialize(1).as<int>() == 1);

    CHECK(kl::json::dump(std::string{"x"}) == R"("x")");
    CHECK(kl::yaml::dump(std::string{"x"}) == "x");

    serialization_record record{7, "text", {1, 2, 3}};

    auto json_value = kl::json::serialize(record);
    REQUIRE(json_value.IsObject());
    CHECK(json_value["i"].GetInt() == 7);
    CHECK(json_value["s"].GetString() == std::string{"text"});
    REQUIRE(json_value["values"].IsArray());
    CHECK(json_value["values"].Size() == 3);

    auto yaml_value = kl::yaml::serialize(record);
    REQUIRE(yaml_value.IsMap());
    CHECK(yaml_value["i"].as<int>() == 7);
    CHECK(yaml_value["s"].as<std::string>() == "text");
    REQUIRE(yaml_value["values"].IsSequence());
    CHECK(yaml_value["values"].size() == 3);
}
