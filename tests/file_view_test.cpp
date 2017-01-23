#include "kl/file_view.hpp"

#include <catch/catch.hpp>
#include <fstream>
#include <string>

TEST_CASE("file_view")
{
    {
        std::ofstream strm{"test.tmp",
                           std::ios::trunc | std::ios::out | std::ios::binary};
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
