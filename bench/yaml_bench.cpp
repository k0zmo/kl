#include "kl/yaml.hpp"
#include "input/typedefs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <yaml-cpp/yaml.h>

#include <sstream>
#include <string>
#include <vector>

static std::string to_string(const YAML::Node& node)
{
    std::ostringstream ostrm;
    ostrm << node;
    return ostrm.str();
}

TEST_CASE("yaml bench")
{
    using namespace kl;
    using Catch::Benchmark::Chronometer;

    constexpr int num_objects = 100;

    BENCHMARK("yaml-cpp's manual serialization")
    {
        YAML::Node doc{YAML::NodeType::Sequence};

        for (int i = 0; i < num_objects; ++i)
        {
            YAML::Node root{YAML::NodeType::Map};
            root["hello"] = "world";
            root["t"] = true;
            root["f"] = false;
            // skip n
            root["i"] = 123;
            root["pi"] = 3.1416f;

            YAML::Node a{YAML::NodeType::Sequence};
            a.push_back(1);
            a.push_back(2);
            a.push_back(3);
            a.push_back(4);
            root["a"] = std::move(a);

            YAML::Node ad1{YAML::NodeType::Sequence};
            ad1.push_back(1);
            ad1.push_back(2);
            YAML::Node ad2{YAML::NodeType::Sequence};
            ad2.push_back(3);
            ad2.push_back(4);
            ad2.push_back(5);
            YAML::Node ad{YAML::NodeType::Sequence};
            ad.push_back(std::move(ad1));
            ad.push_back(std::move(ad2));
            root["ad"] = std::move(ad);

            root["space"] = "lab";

            YAML::Node tup{YAML::NodeType::Sequence};
            tup.push_back(1);
            tup.push_back(3.14f);
            tup.push_back("QWE");
            root["tup"] = std::move(tup);

            YAML::Node map{YAML::NodeType::Map};
            map["1"] = "hls";
            map["2"] = "rgb";
            root["map"] = std::move(map);

            YAML::Node inner{YAML::NodeType::Map};
            inner["r"] = 1337;
            inner["d"] = 3.145926;
            root["inner"] = std::move(inner);

            doc.push_back(std::move(root));
        }

        return to_string(doc);
    };

    BENCHMARK_ADVANCED("yamp::to_map")(Chronometer meter)
    {
        test_t input;

        meter.measure([&] {
            yaml::serialize_context ctx;

            auto doc_builder = yaml::to_sequence(ctx);
            for (int i = 0; i < num_objects; ++i)
            {
                auto map = yaml::to_map(ctx);
                map.add("hello", input.hello)
                    .add("t", input.t)
                    .add("f", input.f)
                    .add("i", input.i)
                    .add("pi", input.pi)
                    .add("a", input.a)
                    .add("ad", input.ad)
                    .add("space", input.space)
                    .add("tup", input.tup)
                    .add("map", input.map)
                    .add("inner", input.inner);
                doc_builder.add(map.done());
            }

            return to_string(doc_builder.done());
        });
    };

    BENCHMARK_ADVANCED("yaml::serialize")(Chronometer meter)
    {
        std::vector<test_t> input;
        for (int i = 0; i < num_objects; ++i)
            input.emplace_back();

        meter.measure([&] {
            auto j = yaml::serialize(input);
            return to_string(j);
        });
    };

    BENCHMARK_ADVANCED("yaml::dump")(Chronometer meter)
    {
        std::vector<test_t> input;
        for (int i = 0; i < num_objects; ++i)
            input.emplace_back();
        meter.measure([&] { return yaml::dump(input); });
    };
}
