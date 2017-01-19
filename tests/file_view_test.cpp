#include "kl/file_view.hpp"

#include <catch/catch.hpp>

TEST_CASE("file_view")
{
    kl::file_view view{"E:\\honda-dywaniki.txt"};
    auto s = view.get_bytes();
}
