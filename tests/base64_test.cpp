#include "kl/base64.hpp"

#include <catch2/catch.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

namespace {

template <std::size_t N>
gsl::span<const std::byte> as_span(const char (&str)[N])
{
    return gsl::span{reinterpret_cast<const std::byte*>(str),
                     N - 1}; // get rid of trailing '\0'
}

template <size_t N>
std::vector<std::byte> as_vector(const char (&str)[N])
{
    return std::vector<std::byte>{reinterpret_cast<const std::byte*>(str),
                                  reinterpret_cast<const std::byte*>(str) + N -
                                      1}; // get rid of trailing '\0'
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
        REQUIRE(base64_encode(as_span("<<???>>")) == "PDw/Pz8+Pg==");
    }

    SECTION("decode")
    {
        REQUIRE(base64_decode("SGVsbG8=").value() == as_vector("Hello"));
        REQUIRE(!base64_decode("SGVsbG8=="));
        REQUIRE(base64_decode("SGVsbG8g").value() == as_vector("Hello "));
        REQUIRE(base64_decode("SGVsbG8gVw==").value() == as_vector("Hello W"));
        REQUIRE(base64_decode("SGVsbG8gV28=").value() == as_vector("Hello Wo"));
        REQUIRE(base64_decode("SGVsbG8gV29y").value() ==
                as_vector("Hello Wor"));
        REQUIRE(base64_decode("SGVsbG8gV29ybA==").value() ==
                as_vector("Hello Worl"));
        REQUIRE(base64_decode("SGVsbG8gV29ybGQ=").value() ==
                as_vector("Hello World"));
        REQUIRE(base64_decode("SGVsbG8gV29ybGQh").value() ==
                as_vector("Hello World!"));
    }

    SECTION("range>127")
    {
        std::string sps = "Z0KAH5ZSAUB7YCoQAAADABAAAAMDzgYABJPgABGMP8Y4wMAAknwA"
                          "AjGH+McO0KFSQA==";
        std::string pps = "aMuNSA==";

        auto sps_decoded = base64_decode(sps);
        auto pps_decoded = base64_decode(pps);

        REQUIRE(base64_encode(sps_decoded.value()) == sps);
        REQUIRE(base64_encode(pps_decoded.value()) == pps);
    }

    SECTION("malformed")
    {
        REQUIRE(!base64_decode("a"));
        REQUIRE(!!base64_decode("aaaa"));
        REQUIRE(!base64_decode("aa=a"));
        REQUIRE(!base64_decode("a==="));
        REQUIRE(!base64_decode("a!=="));
        REQUIRE(!base64_decode("a@!="));
        REQUIRE(!base64_decode("aa-a"));
        REQUIRE(!!base64_decode("aa+a"));
    }
}

TEST_CASE("base64 URL variant")
{
    using namespace kl;

    SECTION("encode")
    {
        REQUIRE(base64url_encode(as_span("Hello")) == "SGVsbG8");
        REQUIRE(base64url_encode(as_span("Hello ")) == "SGVsbG8g");
        REQUIRE(base64url_encode(as_span("Hello W")) == "SGVsbG8gVw");
        REQUIRE(base64url_encode(as_span("Hello Wo")) == "SGVsbG8gV28");
        REQUIRE(base64url_encode(as_span("Hello Wor")) == "SGVsbG8gV29y");
        REQUIRE(base64url_encode(as_span("Hello Worl")) == "SGVsbG8gV29ybA");
        REQUIRE(base64url_encode(as_span("Hello World")) == "SGVsbG8gV29ybGQ");
        REQUIRE(base64url_encode(as_span("Hello World!")) == "SGVsbG8gV29ybGQh");
        REQUIRE(base64url_encode(as_span("<<???>>")) == "PDw_Pz8-Pg");
    }

    SECTION("decode")
    {
        REQUIRE(base64url_decode("SGVsbG8").value() == as_vector("Hello"));
        REQUIRE(base64url_decode("SGVsbG8g").value() == as_vector("Hello "));
        REQUIRE(base64url_decode("SGVsbG8gVw").value() == as_vector("Hello W"));
        REQUIRE(base64url_decode("SGVsbG8gV28").value() ==
                as_vector("Hello Wo"));
        REQUIRE(base64url_decode("SGVsbG8gV29y").value() ==
                as_vector("Hello Wor"));
        REQUIRE(base64url_decode("SGVsbG8gV29ybA").value() ==
                as_vector("Hello Worl"));
        REQUIRE(base64url_decode("SGVsbG8gV29ybGQ").value() ==
                as_vector("Hello World"));
        REQUIRE(base64url_decode("SGVsbG8gV29ybGQh").value() ==
                as_vector("Hello World!"));
    }

    SECTION("ignore '=' on the end")
    {
        REQUIRE(base64url_decode("SGVsbG8gVw==").value() ==
                as_vector("Hello W"));
        REQUIRE(base64url_decode("SGVsbG8gVw=").value() ==
                as_vector("Hello W"));
        REQUIRE(base64url_decode("SGVsbG8gVw").value() == as_vector("Hello W"));
        REQUIRE(!base64url_decode("SGVsbG8gVw==="));
    }

    SECTION("range>127")
    {
        std::string sps = "Z0KAH5ZSAUB7YCoQAAADABAAAAMDzgYABJPgABGMP8Y4wMAAknwA"
                          "AjGH-McO0KFSQA==";
        std::string pps = "aMuNSA==";

        auto sps_decoded = base64url_decode(sps);
        auto pps_decoded = base64url_decode(pps);

        REQUIRE(base64url_encode(sps_decoded.value()) ==
                sps.substr(0, sps.length() - 2));
        REQUIRE(base64url_encode(pps_decoded.value()) ==
                pps.substr(0, pps.length() - 2));
    }

    SECTION("malformed")
    {
        REQUIRE(!base64url_decode("a"));
        REQUIRE(!!base64url_decode("aaaa"));
        REQUIRE(!base64url_decode("aa=a"));
        REQUIRE(!base64url_decode("a==="));
        REQUIRE(!base64url_decode("a!=="));
        REQUIRE(!base64url_decode("a@!="));
        REQUIRE(!base64url_decode("aa+a"));
        REQUIRE(!!base64url_decode("aa-a"));
    }
}
