#include <kl/path.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("path parses URL-style strings", "[path]")
{
    const kl::path path{"/a/./b/../name%3Asurname"};

    REQUIRE(path.size() == 2);
    CHECK_FALSE(path.empty());
    CHECK(path[0] == "a");
    CHECK(path[1] == "name:surname");
    CHECK(path.joined(0) == "a/name:surname");
    CHECK(path.joined(1) == "name:surname");
    CHECK(path.joined(2).empty());

    CHECK(kl::path{""}.empty());
    CHECK(kl::path{"/"}.empty());
    CHECK(kl::path{"/a//b/"}[1] == "b");
    CHECK(kl::path{"/a/../b"}[0] == "b");
    CHECK(kl::path{"/a/%2E/%2E%2E"}[2] == "..");

    CHECK_THROWS_AS(kl::path{"/a/%"}, kl::path_parse_error);
    CHECK_THROWS_AS(kl::path{"/a/%0G"}, kl::path_parse_error);
    CHECK_THROWS_AS(kl::path{"/items/name%2Fsurname"}, kl::path_parse_error);
    CHECK_THROWS_AS(kl::path{"/items/name%2fsurname"}, kl::path_parse_error);
}

TEST_CASE("path can be reset", "[path]")
{
    kl::path path;

    CHECK(path.empty());
    CHECK(path.size() == 0);
    CHECK(path.joined(0).empty());

    path.set("/a/b/c");
    REQUIRE_FALSE(path.empty());
    REQUIRE(path.size() == 3);
    CHECK(path[0] == "a");
    CHECK(path[2] == "c");
    CHECK(path.joined(1) == "b/c");

    path.clear();
    CHECK(path.empty());
    CHECK(path.size() == 0);
}

TEST_CASE("path view exposes segments", "[path]")
{
    const kl::path path{"/a/b/c"};
    const auto view = path.view();

    REQUIRE_FALSE(view.empty());
    REQUIRE(view.size() == 3);
    CHECK(view[1] == "b");
    CHECK(view.front() == "a");
    CHECK(view.prefix(0).empty());
    CHECK(view.prefix(1) == "a");
    CHECK(view.prefix(3) == "a/b/c");

    const auto tail = view.drop_front();
    REQUIRE(tail.size() == 2);
    CHECK(tail.front() == "b");
    CHECK(tail.prefix(2) == "a/b/c");

    const auto head = view.drop_back();
    REQUIRE(head.size() == 2);
    CHECK(head[1] == "b");
    CHECK(head.prefix(2) == "a/b");

    const kl::path_view empty;
    CHECK(empty.empty());
    CHECK(empty.size() == 0);
    CHECK(empty.prefix(0).empty());
}
