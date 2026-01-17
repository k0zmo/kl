#include <kl/signal.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <future>
#include <vector>

namespace {

class Target
{
public:
    void operator()(int arg)
    {
        total += arg;
    }

    int total = 0;
};

class AtomicTarget
{
public:
    AtomicTarget() = default;
    AtomicTarget(const AtomicTarget& other)
    {
        total = other.total.load();
    }

    void operator()(int arg)
    {
        total += arg;
    }

    std::atomic<int> total = 0;
};

template <std::size_t N>
auto connect_benchmark(Catch::Benchmark::Chronometer& meter)
{
    std::vector<Target> targets(N);

    meter.measure([&] {
        kl::signal<void(int)> sig;
        for (auto& tgt : targets)
            sig.connect(std::ref(tgt));
    });

    return targets.back().total;
}

template <std::size_t N>
auto emit_benchmark(Catch::Benchmark::Chronometer& meter)
{
    std::vector<Target> targets(N);
    kl::signal<void(int)> sig;

    for (auto& tgt : targets)
        sig.connect(std::ref(tgt));

    meter.measure([&]{
        sig(42);
    });

    return targets.back().total;
}

template <std::size_t N>
auto disconnect_benchmark(Catch::Benchmark::Chronometer& meter)
{
    std::vector<Target> targets(N);
    std::vector<std::vector<kl::connection>> connections(meter.runs());
    std::vector<kl::signal<void(int)>> sig(meter.runs());
    for (int i = 0; i < meter.runs(); ++i)
    {
        for (auto& tgt : targets)
            connections[i].push_back(sig[i].connect(std::ref(tgt)));
    }

    meter.measure([&](int i) {
        for (auto& conn : connections[i])
            conn.disconnect();
        return targets.back().total;
    });
}

template <std::size_t N, std::size_t NUM_THREADS>
auto threaded_connect_emit_benchmark()
{
    kl::signal<void(int)> sig;

    auto ctx = [&] {
        std::vector<AtomicTarget> targets(N);
        std::vector<kl::scoped_connection> connections;
        for (auto& tgt : targets)
            connections.push_back(sig.connect(std::ref(tgt)));
        sig(42);
        return targets.back().total.load();
    };

    std::vector<std::future<int>> results;
    for (auto i = 0U; i < NUM_THREADS; ++i)
    {
        results.emplace_back(std::async(std::launch::async, ctx));
    }

    int total = 0;
    for (auto& fut : results)
        total += fut.get();
    return total;
}

} // namespace

using Catch::Benchmark::Chronometer;

TEST_CASE("signal bench - connect")
{
    BENCHMARK_ADVANCED("connect x1")(Chronometer meter)
    {
        return connect_benchmark<1>(meter);
    };
    BENCHMARK_ADVANCED("connect x2")(Chronometer meter)
    {
        return connect_benchmark<2>(meter);
    };
    BENCHMARK_ADVANCED("connect x4")(Chronometer meter)
    {
        return connect_benchmark<4>(meter);
    };
    BENCHMARK_ADVANCED("connect x8")(Chronometer meter)
    {
        return connect_benchmark<8>(meter);
    };
    BENCHMARK_ADVANCED("connect x16")(Chronometer meter)
    {
        return connect_benchmark<16>(meter);
    };
    BENCHMARK_ADVANCED("connect x32")(Chronometer meter)
    {
        return connect_benchmark<32>(meter);
    };
    BENCHMARK_ADVANCED("connect x64")(Chronometer meter)
    {
        return connect_benchmark<64>(meter);
    };
}

TEST_CASE("signal bench - emit")
{
    BENCHMARK_ADVANCED("emit to 1")(Chronometer meter)
    {
        return emit_benchmark<1>(meter);
    };
    BENCHMARK_ADVANCED("emit to 2")(Chronometer meter)
    {
        return emit_benchmark<2>(meter);
    };
    BENCHMARK_ADVANCED("emit to 4")(Chronometer meter)
    {
        return emit_benchmark<4>(meter);
    };
    BENCHMARK_ADVANCED("emit to 8")(Chronometer meter)
    {
        return emit_benchmark<8>(meter);
    };
    BENCHMARK_ADVANCED("emit to 16")(Chronometer meter)
    {
        return emit_benchmark<16>(meter);
    };
    BENCHMARK_ADVANCED("emit to 32")(Chronometer meter)
    {
        return emit_benchmark<32>(meter);
    };
    BENCHMARK_ADVANCED("emit to 64")(Chronometer meter)
    {
        return emit_benchmark<64>(meter);
    };
}

TEST_CASE("signal bench - disconnect")
{
    BENCHMARK_ADVANCED("disconnect from 1")(Chronometer meter)
    {
        return disconnect_benchmark<1>(meter);
    };
    BENCHMARK_ADVANCED("disconnect from 2")(Chronometer meter)
    {
        return disconnect_benchmark<2>(meter);
    };
    BENCHMARK_ADVANCED("disconnect from 4")(Chronometer meter)
    {
        return disconnect_benchmark<4>(meter);
    };
    BENCHMARK_ADVANCED("disconnect from 8")(Chronometer meter)
    {
        return disconnect_benchmark<8>(meter);
    };
    BENCHMARK_ADVANCED("disconnect from 16")(Chronometer meter)
    {
        return disconnect_benchmark<16>(meter);
    };
    BENCHMARK_ADVANCED("disconnect from 32")(Chronometer meter)
    {
        return disconnect_benchmark<32>(meter);
    };
    BENCHMARK_ADVANCED("disconnect from 64")(Chronometer meter)
    {
        return disconnect_benchmark<64>(meter);
    };
}

// TEST_CASE("signal bench - threaded")
// {
//     BENCHMARK("threaded (N=2) connect and emit to x1")
//     {
//         return threaded_connect_emit_benchmark<2, 1>();
//     };
//     BENCHMARK("threaded (N=2) connect and emit to x2")
//     {
//         return threaded_connect_emit_benchmark<2, 2>();
//     };
//     BENCHMARK("threaded (N=2) connect and emit to x4")
//     {
//         return threaded_connect_emit_benchmark<2, 4>();
//     };
//     BENCHMARK("threaded (N=2) connect and emit to x8")
//     {
//         return threaded_connect_emit_benchmark<2, 8>();
//     };
//     BENCHMARK("threaded (N=2) connect and emit to x16")
//     {
//         return threaded_connect_emit_benchmark<2, 16>();
//     };
//     BENCHMARK("threaded (N=2) connect and emit to x32")
//     {
//         return threaded_connect_emit_benchmark<2, 32>();
//     };
//     BENCHMARK("threaded (N=2) connect and emit to x64")
//     {
//         return threaded_connect_emit_benchmark<2, 64>();
//     };

//     BENCHMARK("threaded (N=4) connect and emit to x1")
//     {
//         return threaded_connect_emit_benchmark<4, 1>();
//     };
//     BENCHMARK("threaded (N=4) connect and emit to x2")
//     {
//         return threaded_connect_emit_benchmark<4, 2>();
//     };
//     BENCHMARK("threaded (N=4) connect and emit to x4")
//     {
//         return threaded_connect_emit_benchmark<4, 4>();
//     };
//     BENCHMARK("threaded (N=4) connect and emit to x8")
//     {
//         return threaded_connect_emit_benchmark<4, 8>();
//     };
//     BENCHMARK("threaded (N=4) connect and emit to x16")
//     {
//         return threaded_connect_emit_benchmark<4, 16>();
//     };
//     BENCHMARK("threaded (N=4) connect and emit to x32")
//     {
//         return threaded_connect_emit_benchmark<4, 32>();
//     };
//     BENCHMARK("threaded (N=4) connect and emit to x64")
//     {
//         return threaded_connect_emit_benchmark<4, 64>();
//     };

//     BENCHMARK("threaded (N=8) connect and emit to x1")
//     {
//         return threaded_connect_emit_benchmark<8, 1>();
//     };
//     BENCHMARK("threaded (N=8) connect and emit to x2")
//     {
//         return threaded_connect_emit_benchmark<8, 2>();
//     };
//     BENCHMARK("threaded (N=8) connect and emit to x4")
//     {
//         return threaded_connect_emit_benchmark<8, 4>();
//     };
//     BENCHMARK("threaded (N=8) connect and emit to x8")
//     {
//         return threaded_connect_emit_benchmark<8, 8>();
//     };
//     BENCHMARK("threaded (N=8) connect and emit to x16")
//     {
//         return threaded_connect_emit_benchmark<8, 16>();
//     };
//     BENCHMARK("threaded (N=8) connect and emit to x32")
//     {
//         return threaded_connect_emit_benchmark<8, 32>();
//     };
//     BENCHMARK("threaded (N=8) connect and emit to x64")
//     {
//         return threaded_connect_emit_benchmark<8, 64>();
//     };
// }
