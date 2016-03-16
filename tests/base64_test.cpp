#include "kl/base64.hpp"

#include <catch/catch.hpp>
#include <boost/optional.hpp>

namespace {
template <std::size_t N>
gsl::span<const kl::byte> as_span(const char (&str)[N])
{
    return gsl::span<const kl::byte>(reinterpret_cast<const kl::byte*>(str),
                                     N - 1); // get rid of trailing '\0'
}

template <size_t N>
std::vector<std::uint8_t> as_vector(const char (&str)[N])
{
    return std::vector<std::uint8_t>{reinterpret_cast<const kl::byte*>(str),
                                     reinterpret_cast<const kl::byte*>(str) +
                                         N - 1}; // get rid of trailing '\0'
}
} // namespace anonymous

TEST_CASE("base64")
{
    using namespace kl;

    SECTION("encode")
    {
        REQUIRE(base64_encode(as_span("Hello")) == "SGVsbG8=");
        REQUIRE(base64_encode(as_span("Hello ")) == "SGVsbG8g");
        REQUIRE(base64_encode(as_span("Hello W")) == "SGVsbG8gVw==");
        REQUIRE(base64_encode(as_span("Hello Wo")) == "SGVsbG8gV28=");
        REQUIRE(base64_encode(as_span("Hello Wor")) == "SGVsbG8gV29y");
        REQUIRE(base64_encode(as_span("Hello Worl")) == "SGVsbG8gV29ybA==");
        REQUIRE(base64_encode(as_span("Hello World")) == "SGVsbG8gV29ybGQ=");
        REQUIRE(base64_encode(as_span("Hello World!")) == "SGVsbG8gV29ybGQh");
    }

    SECTION("decode")
    {
        REQUIRE(base64_decode("SGVsbG8="));
        REQUIRE(base64_decode("SGVsbG8=").get() == as_vector("Hello"));

        REQUIRE(base64_decode("SGVsbG8g"));
        REQUIRE(base64_decode("SGVsbG8g").get() == as_vector("Hello "));

        REQUIRE(base64_decode("SGVsbG8gVw=="));
        REQUIRE(base64_decode("SGVsbG8gVw==").get() == as_vector("Hello W"));

        REQUIRE(base64_decode("SGVsbG8gV28="));
        REQUIRE(base64_decode("SGVsbG8gV28=").get() == as_vector("Hello Wo"));

        REQUIRE(base64_decode("SGVsbG8gV29y"));
        REQUIRE(base64_decode("SGVsbG8gV29y").get() == as_vector("Hello Wor"));

        REQUIRE(base64_decode("SGVsbG8gV29ybA=="));
        REQUIRE(base64_decode("SGVsbG8gV29ybA==").get() == as_vector("Hello Worl"));

        REQUIRE(base64_decode("SGVsbG8gV29ybGQ="));
        REQUIRE(base64_decode("SGVsbG8gV29ybGQ=").get() == as_vector("Hello World"));

        REQUIRE(base64_decode("SGVsbG8gV29ybGQh"));
        REQUIRE(base64_decode("SGVsbG8gV29ybGQh").get() == as_vector("Hello World!"));
    }

    SECTION("range>127")
    {
        std::string sps = "Z0KAH5ZSAUB7YCoQAAADABAAAAMDzgYABJPgABGMP8Y4wMAAknwA"
                          "AjGH+McO0KFSQA==";
        std::string pps = "aMuNSA==";

        auto sps_decoded = base64_decode(sps);
        auto pps_decoded = base64_decode(pps);

        REQUIRE(base64_encode(sps_decoded.get()) == sps);
        REQUIRE(base64_encode(pps_decoded.get()) == pps);
    }

    SECTION("malformed")
    {
        REQUIRE(!base64_decode("a"));
        REQUIRE(base64_decode("aaaa"));
        REQUIRE(!base64_decode("aa=a"));
        REQUIRE(!base64_decode("a==="));
    }
}
