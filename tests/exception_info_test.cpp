#include "kl/exception_info.hpp"
#include "kl/split.hpp"

#include <catch2/catch.hpp>

#include <exception>
#include <vector>
#include <stdexcept>
#include <cstring>

namespace {

// clang-format off
struct io_error: std::exception {};
struct read_error: virtual io_error {};
struct write_error: virtual io_error {};
struct file_error: virtual io_error {};

struct xi_file_name { using type = std::string; };
struct xi_errno { using type = int; };
struct xi_vec { using type = std::vector<int>;  };

struct int_struct { int value; };
struct xi_int { using type = int_struct; };
// clang-format on
} // namespace

TEST_CASE("exception_info - default constructed")
{
    kl::exception_info xi;

    CHECK_FALSE(xi.file());
    CHECK(xi.line() == 0);
    CHECK_FALSE(xi.function());
    CHECK_FALSE(xi.get<xi_errno>());
    CHECK_FALSE(xi.get<xi_file_name>());

    CHECK(xi.diagnostic_info() ==
          "<unknown-file>: throw_with_info in function <unknown-function>\n");

    kl::exception_info xi2 = xi;
    xi = std::move(xi2);

    xi.unset<xi_errno>();
    xi.unset<xi_file_name>();
}

TEST_CASE("exception_info ")
{
    auto xi = KL_MAKE_EXCEPTION_INFO();

    CHECK_FALSE(std::strcmp(xi.file(), __FILE__));
    CHECK(xi.line() < __LINE__);
    CHECK_FALSE(std::strcmp(xi.function(), KL_FUNCTION_NAME));
    CHECK_FALSE(xi.get<xi_errno>());
    CHECK_FALSE(xi.get<xi_file_name>());

    CHECK(xi.diagnostic_info() !=
          "<unknown-file>: throw_with_info in function <unknown-function>\n");

    SECTION("stack like behaviour")
    {
        xi.set<xi_errno>(123);
        REQUIRE(xi.get<xi_errno>());
        CHECK(*xi.get<xi_errno>() == 123);

        xi.set<xi_errno>(256);
        REQUIRE(xi.get<xi_errno>());
        CHECK(*xi.get<xi_errno>() == 256);

        xi.unset<xi_errno>();
        REQUIRE(xi.get<xi_errno>());
        CHECK(*xi.get<xi_errno>() == 123);
    }

    SECTION("attach two types")
    {
        xi.set<xi_errno>(123);
        xi.set<xi_file_name>("aaa");

        REQUIRE(xi.get<xi_errno>());
        CHECK(*xi.get<xi_errno>() == 123);
        REQUIRE(xi.get<xi_file_name>());
        CHECK(*xi.get<xi_file_name>() == "aaa");

        SECTION("detach first errno")
        {
            xi.unset<xi_errno>();
            REQUIRE_FALSE(xi.get<xi_errno>());
            REQUIRE(xi.get<xi_file_name>());
            CHECK(*xi.get<xi_file_name>() == "aaa");

            xi.unset<xi_file_name>();

            const auto& xi2 = xi;
            REQUIRE_FALSE(xi2.get<xi_errno>());
            REQUIRE_FALSE(xi2.get<xi_file_name>());
        }

        SECTION("detach first file_name")
        {
            xi.unset<xi_file_name>();
            REQUIRE_FALSE(xi.get<xi_file_name>());
            REQUIRE(xi.get<xi_errno>());
            CHECK(*xi.get<xi_errno>() == 123);

            xi.unset<xi_errno>();
            REQUIRE_FALSE(xi.get<xi_errno>());
            REQUIRE_FALSE(xi.get<xi_file_name>());
        }
    }

    SECTION("diagnostic info")
    {
        auto s = xi.diagnostic_info();
        REQUIRE(kl::split(s, "\n").size() == 1);

        xi.set<xi_errno>(123);
        s = xi.diagnostic_info();
        REQUIRE(kl::split(s, "\n").size() == 2);

        xi.set<xi_file_name>("aaa");
        s = xi.diagnostic_info();
        REQUIRE(kl::split(s, "\n").size() == 3);

        xi.set<xi_vec>({});
        s = xi.diagnostic_info();
        REQUIRE(kl::split(s, "\n").size() == 4);

        xi.set<xi_int>({(int)0xabcdef12});
        s = xi.diagnostic_info();
        REQUIRE(kl::split(s, "\n").size() == 5);
    }
}

TEST_CASE("throw_with_info")
{
    bool thrown{};

    try
    {
        try
        {
            kl::throw_with_info(
                read_error{},
                kl::exception_info(__FILE__, __LINE__, KL_FUNCTION_NAME)
                    .set<xi_errno>(1234));
        }
        catch (kl::exception_info& xi)
        {
            REQUIRE(xi.get<xi_errno>());
            xi.set<xi_file_name>("FILE_NAME.txt");
            throw;
        }
    }
    catch (const io_error& ex)
    {
        thrown = true;

        const kl::exception_info* xi = kl::get_exception_info(ex);
        REQUIRE(xi);

        auto err = xi->get<xi_errno>();
        REQUIRE(err);
        CHECK(*err == 1234);

        auto fn = xi->get<xi_file_name>();
        REQUIRE(fn);
        CHECK(*fn == "FILE_NAME.txt");

        try
        {
            throw;
        }
        catch (io_error& ex)
        {
            kl::exception_info* xi2 = kl::get_exception_info(ex);
            REQUIRE(xi2);
        }
    }

    CHECK(thrown);
}

TEST_CASE("exception_diagnostic_info")
{
    SECTION("throw primitive")
    {
        try
        {
            throw 1;
        }
        catch (...)
        {
            auto s = kl::exception_diagnostic_info(std::current_exception());
            REQUIRE(s == "dynamic exception type: <unknown-exception>\n");
        }
    }

    SECTION("throw std::string")
    {
        try
        {
            throw std::string{};
        }
        catch (std::string& ex)
        {
            auto s = kl::exception_diagnostic_info(ex);
            REQUIRE(s != "dynamic exception type: <unknown-exception>\n");
        }
    }

    SECTION("throw runtime error")
    {
        try
        {
            throw std::runtime_error{"test error"};
        }
        catch (...)
        {
            auto s = kl::exception_diagnostic_info(std::current_exception());
            REQUIRE_FALSE(s.empty());
            auto ss = kl::split(s, "\n");
            REQUIRE(ss.size() == 2);
            REQUIRE(ss[1] == "what: test error");
        }
    }


    SECTION("throw with info")
    {
        try
        {
            kl::throw_with_info(read_error(),
                                KL_MAKE_EXCEPTION_INFO().set<xi_errno>(555));
        }
        catch (...)
        {
            auto s = kl::exception_diagnostic_info(std::current_exception());
            REQUIRE(kl::split(s, "\n").size() == 4);
        }
    }
}
