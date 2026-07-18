#include "kl/base64.hpp"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>
#include <catch2/catch_test_macros.hpp>
#include <gsl/span>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string make_binary_text(std::size_t size)
{
    std::string ret(size, '\0');
    for (std::size_t i = 0; i < ret.size(); ++i)
        ret[i] = static_cast<char>((i * 131U + 17U) & 0xFFU);
    return ret;
}

gsl::span<const std::byte> as_bytes(std::string_view text)
{
    return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

std::string to_string(const std::vector<std::byte>& bytes)
{
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void run_base64_benchmarks(std::string_view name, const std::string& input)
{
    using Catch::Benchmark::Chronometer;

    const auto kl_encoded = kl::base64_encode(as_bytes(input));

    REQUIRE(kl::base64_decode(kl_encoded));
    REQUIRE(to_string(kl::base64_decode(kl_encoded).value()) == input);

    BENCHMARK_ADVANCED(std::string{"kl::base64_encode/"} + std::string{name})(
        Chronometer meter)
    {
        meter.measure([&] { return kl::base64_encode(as_bytes(input)); });
    };

    BENCHMARK_ADVANCED(std::string{"kl::base64_decode/"} + std::string{name})(
        Chronometer meter)
    {
        meter.measure([&] { return kl::base64_decode(kl_encoded); });
    };
}

} // namespace

TEST_CASE("base64 bench")
{
    run_base64_benchmarks("4 KiB", make_binary_text(4U * 1024U));
    run_base64_benchmarks("64 KiB", make_binary_text(64U * 1024U));
    run_base64_benchmarks("256 KiB", make_binary_text(256U * 1024U));
}
