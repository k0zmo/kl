#include "kl/json.hpp"
#include "input/typedefs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <rapidjson/document.h>
#include <rapidjson/allocators.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <vector>

static std::string to_string(const rapidjson::Value& v)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    v.Accept(writer);
    return std::string(buffer.GetString());
}

TEST_CASE("json bench")
{
    using namespace kl;
    using Catch::Benchmark::Chronometer;

    constexpr int num_objects = 100;

    BENCHMARK("rapidjson's manual serialization")
    {
        rapidjson::Document doc{rapidjson::kArrayType};
        auto& alloc = doc.GetAllocator();
        rapidjson::Value& root = doc;

        for (int i = 0; i < num_objects; ++i)
        {
            rapidjson::Value obj{rapidjson::kObjectType};
            obj.AddMember("hello", "world", alloc);
            obj.AddMember("t", true, alloc);
            obj.AddMember("f", false, alloc);
            // skip n
            obj.AddMember("i", 123, alloc);
            obj.AddMember("pi", 3.1416f, alloc);

            rapidjson::Value a{rapidjson::kArrayType};
            a.PushBack(1, alloc);
            a.PushBack(2, alloc);
            a.PushBack(3, alloc);
            a.PushBack(4, alloc);
            obj.AddMember("a", std::move(a), alloc);

            rapidjson::Value ad1{rapidjson::kArrayType};
            ad1.PushBack(1, alloc);
            ad1.PushBack(2, alloc);
            rapidjson::Value ad2{rapidjson::kArrayType};
            ad2.PushBack(3, alloc);
            ad2.PushBack(4, alloc);
            ad2.PushBack(5, alloc);
            rapidjson::Value ad{rapidjson::kArrayType};
            ad.PushBack(std::move(ad1), alloc);
            ad.PushBack(std::move(ad2), alloc);
            obj.AddMember("ad", std::move(ad), alloc);

            obj.AddMember("space", "lab", alloc);

            rapidjson::Value tup{rapidjson::kArrayType};
            tup.PushBack(1, alloc);
            tup.PushBack(3.14f, alloc);
            tup.PushBack("QWE", alloc);
            obj.AddMember("tup", std::move(tup), alloc);

            rapidjson::Value map{rapidjson::kObjectType};
            map.AddMember("1", "hls", alloc);
            map.AddMember("2", "rgb", alloc);
            obj.AddMember("map", std::move(map), alloc);

            rapidjson::Value inner{rapidjson::kObjectType};
            inner.AddMember("r", 1337, alloc);
            inner.AddMember("d", 3.145926, alloc);
            obj.AddMember("inner", std::move(inner), alloc);

            root.PushBack(std::move(obj), alloc);
        }

        return to_string(doc);
    };

    BENCHMARK_ADVANCED("json::to_object")(Chronometer meter)
    {
        test_t input;

        meter.measure([&] {
            json::owning_serialize_context ctx;

            auto doc_builder = json::to_array(ctx);
            for (int i = 0; i < num_objects; ++i)
            {
                auto obj = json::to_object(ctx);
                obj.add("hello", input.hello)
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
                doc_builder.add(obj.done());
            }

            return to_string(doc_builder.done());
        });
    };

    BENCHMARK_ADVANCED("json::serialize")(Chronometer meter)
    {
        std::vector<test_t> input;
        for (int i = 0; i < num_objects; ++i)
            input.emplace_back();

        meter.measure([&] {
            auto j = json::serialize(input);
            return to_string(j);
        });
    };

    BENCHMARK_ADVANCED("json::dump")(Chronometer meter)
    {
        std::vector<test_t> input;
        for (int i = 0; i < num_objects; ++i)
            input.emplace_back();
        meter.measure([&] { return json::dump(input); });
    };
}
