#include "kl/file_view.hpp"

#include <catch2/catch.hpp>
#include <fstream>
#include <string>

TEST_CASE("file_view")
{
    SECTION("file not found")
    {
        REQUIRE_THROWS_AS(kl::file_view{"test22_does_not_exist.tmp"},
                          std::runtime_error);
    }

    SECTION("read empty file")
    {
        {
            std::ofstream{"test_empty_file.tmp",
                          std::ios::trunc | std::ios::out};
        }

        kl::file_view view{"test_empty_file.tmp"};
        auto s = view.get_bytes();

        REQUIRE(s.empty());
    }

    SECTION("read file")
    {
        {
            std::ofstream strm{"test.tmp", std::ios::trunc | std::ios::out |
                                               std::ios::binary};
            strm << "Test\nHello.";
        }

        kl::file_view view{"test.tmp"};
        auto s = view.get_bytes();

        REQUIRE(s.size_bytes() == 11);

        std::string str;
        str.resize(s.size_bytes());
        std::copy_n(s.data(), s.size_bytes(), str.begin());

        REQUIRE(str == "Test\nHello.");
    }
}
