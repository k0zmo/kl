#include "kl/json.hpp"
#include "kl/reflect_struct.hpp"
#include "kl/yaml.hpp"

#include "input/typedefs.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <vector>

namespace kl {
namespace serialization {

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

} // namespace serialization
} // namespace kl

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

TEST_CASE("serialization - chrono duration serializer works with json and yaml")
{
    using namespace std::chrono;

    chrono_test serialized{2, seconds{10}, {seconds{10}, seconds{10}}};

    auto json_value = kl::json::serialize(serialized);
    auto json_out = kl::json::deserialize<chrono_test>(json_value);
    CHECK(json_out.t == 2);
    CHECK(json_out.sec == seconds{10});
    CHECK(json_out.secs == std::vector<seconds>{seconds{10}, seconds{10}});

    auto yaml_value = kl::yaml::serialize(serialized);
    auto yaml_out = kl::yaml::deserialize<chrono_test>(yaml_value);
    CHECK(yaml_out.t == 2);
    CHECK(yaml_out.sec == seconds{10});
    CHECK(yaml_out.secs == std::vector<seconds>{seconds{10}, seconds{10}});

    chrono_test dumped{2, seconds{10}, {seconds{6}, seconds{12}}};
    CHECK(kl::json::dump(dumped) == R"({"t":2,"sec":10,"secs":[6,12]})");
    CHECK(kl::yaml::dump(dumped) == "t: 2\nsec: 10\nsecs:\n  - 6\n  - 12");
}
