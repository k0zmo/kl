#include "kl/buffer_rw.hpp"
#include "kl/buffer_rw/map.hpp"
#include "kl/buffer_rw/optional.hpp"
#include "kl/buffer_rw/set.hpp"
#include "kl/buffer_rw/string.hpp"
#include "kl/buffer_rw/variant.hpp"
#include "kl/buffer_rw/vector.hpp"

#include <boost/endian/arithmetic.hpp>
#include <catch/catch.hpp>
#include <gsl/string_span>

#include <cstring>

TEST_CASE("buffer_reader")
{
    using namespace kl;

    SECTION("empty span")
    {
        buffer_reader r{gsl::span<const byte>()};

        REQUIRE(r.left() == 0);
        REQUIRE(r.pos() == 0);
        REQUIRE(r.empty());
        REQUIRE(!r.err());

        SECTION("try peek")
        {
            unsigned char c;
            REQUIRE(!r.peek<unsigned char>(c));
            REQUIRE(!r.err());
        }
        
        SECTION("try read")
        {
            unsigned char c;
            REQUIRE(!r.read<unsigned char>(c));
            REQUIRE(r.err());
        }

        SECTION("notify error")
        {
            r.notify_error();
            REQUIRE(r.err());
        }

        SECTION("skip by negative")
        {
            r.skip(-1);
            REQUIRE(r.err());
        }

        SECTION("skip by positive")
        {
            r.skip(1);
            REQUIRE(r.err());
        }
    }

    SECTION("span with 4 bytes")
    {
        std::array<byte, 4> buf = {1, 2, 3, 4};
        buffer_reader r{buf};

        REQUIRE(r.left() == 4);
        REQUIRE(r.peek<byte>() == 1);
        REQUIRE(r.peek<boost::endian::big_uint16_t>() == 0x0102U);
        REQUIRE(r.peek<boost::endian::big_uint32_t>() == 0x01020304U);
        REQUIRE(r.read<byte>() == 1);
        REQUIRE(r.pos() == 1);
        REQUIRE(!r.err());

        REQUIRE(r.peek<byte>() == 2);
        REQUIRE(r.peek<boost::endian::big_uint16_t>() == 0x0203U);
        REQUIRE(r.left() == 3);
        REQUIRE(r.read<boost::endian::big_uint24_t>() == 0x020304U);
        REQUIRE(r.left() == 0);
        REQUIRE(r.pos() == 4);
    }

    SECTION("read cstring as span")
    {
        const char* str = "Hello world!";
        buffer_reader r(gsl::ensure_z(str));

        REQUIRE(r.left() == 12);
        REQUIRE(gsl::as_bytes(r.view(r.left())) ==
                gsl::as_bytes(gsl::ensure_z(str)));
        REQUIRE(r.pos() == 12);
        REQUIRE(r.left() == 0);

        r.skip(-1);
        REQUIRE(r.read<char>() == '!');
        REQUIRE(r.left() == 0);
    }
}

TEST_CASE("buffer_reader - map")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        buffer_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 4 + 4 + 1 + 1> buf = {3, 0, 0, 0, 0, 0,   0,
                                                   0, 1, 0, 0, 0, '@', 0};
        buffer_reader r{buf};

        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("2 elems")
    {
        std::array<byte, 4 + 1 + 4 + 1 + 4> buf = {2, 0, 0, 0,  2, 100, 0,
                                                   0, 0, 1, 10, 0, 0,   0};
        buffer_reader r{buf};

        auto ret = r.read<std::map<byte, int>>();
        REQUIRE(ret.size() == 2);
        REQUIRE(ret[1] == 10);
        REQUIRE(ret[2] == 100);
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("buffer_reader - set")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        buffer_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::set<int>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 2> buf = {3, 0, 0, 0, 1, 2};
        buffer_reader r{buf};

        auto ret = r.read<std::set<std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("3 elems")
    {
        std::array<byte, 4 + 3> buf = {3, 0, 0, 0, 1, 2, 3};
        buffer_reader r{buf};

        auto ret = r.read<std::set<byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret.find(1) != ret.end());
        REQUIRE(ret.find(2) != ret.end());
        REQUIRE(ret.find(3) != ret.end());
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("buffer_reader - optional")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        buffer_reader r{gsl::span<const byte>{}};
        auto ret = r.read<boost::optional<int>>();
        REQUIRE(r.err());
    }

    SECTION("nullopt")
    {
        std::array<byte, 1> buf = {0};
        buffer_reader r{buf};

        auto ret = r.read<boost::optional<int>>();
        REQUIRE(!ret);
        REQUIRE(!r.err());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 2> buf = {1, 0};
        buffer_reader r{buf};

        auto ret = r.read<boost::optional<int>>();
        REQUIRE(r.err());
    }

    SECTION("ok")
    {
        std::array<byte, 1 + 4 + 1> buf = {1, 1, 0, 0, 0, '!'};
        buffer_reader r{buf};

        auto ret = r.read<boost::optional<std::string>>();
        REQUIRE(!r.err());
        REQUIRE(ret);
        REQUIRE(*ret == "!");
    }
}

TEST_CASE("buffer_reader - string")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        buffer_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::string>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 12> buf = {13,  0,   0,   0,   'H', 'e', 'l', 'l',
                                        'o', ',', ' ', 'w', 'o', 'r', 'l'};
        buffer_reader r{buf};

        std::string ret;
        REQUIRE(!r.read(ret));
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("ok")
    {
        std::array<byte, 4 + 13> buf = {13,  0,   0,   0,   'H', 'e',
                                        'l', 'l', 'o', ',', ' ', 'w',
                                        'o', 'r', 'l', 'd', '!'};
        buffer_reader r{buf};

        auto ret = r.read<std::string>();
        REQUIRE(ret == "Hello, world!");
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("buffer_reader - variant")
{
    using namespace kl;
    using variant = boost::variant<boost::endian::little_int32_t, std::string>;

    SECTION("empty buffer")
    {
        buffer_reader r{gsl::span<const byte>{}};
        auto ret = r.read<variant>();
        REQUIRE(r.err());
    }

    SECTION("invalid 'which'")
    {
        std::array<byte, 1> buf = {3};
        buffer_reader r{buf};
        auto ret = r.read<variant>();
        REQUIRE(r.err());
    }

    SECTION("read as int")
    {
        std::array<byte, 1 + 4> buf = {0, 19, 0, 0, 0};
        buffer_reader r{buf};

        variant var;
        r.read(var);
        REQUIRE(var.which() == 0);
        REQUIRE(boost::get<boost::endian::little_int32_t>(var) == 19);
        REQUIRE(r.empty());
    }

    SECTION("read as int, buffer too short")
    {
        std::array<byte, 1 + 2> buf = {0, 19, 0};
        buffer_reader r{buf};

        variant var;
        REQUIRE(!r.read(var));
    }

    SECTION("read as string")
    {
        std::array<byte, 1 + 4 + 19> buf = {
            1,   19,  0,   0,   0,   'H', 'e', 'l', 'l', 'o', ',', ' ',
            'w', 'o', 'r', 'l', 'd', '!', ' ', ' ', ' ', ' ', ' ', ' '};
        buffer_reader r{buf};

        auto ret = r.read<variant>();
        REQUIRE(ret.which() == 1);
        REQUIRE(boost::get<std::string>(ret) == "Hello, world!      ");
        REQUIRE(r.empty());
    }
}

TEST_CASE("buffer_reader - vector")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        buffer_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::vector<int>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 2> buf = {3, 0, 0, 0, 1, 2};
        buffer_reader r{buf};

        auto ret = r.read<std::vector<std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("3 elems")
    {
        std::array<byte, 4 + 3 + 1> buf = {3, 0, 0, 0, 1, 2, 3, 4};
        buffer_reader r{buf};

        auto ret = r.read<std::vector<byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret[0] == 1);
        REQUIRE(ret[1] == 2);
        REQUIRE(ret[2] == 3);
        REQUIRE(!r.err());
        REQUIRE(r.left() == 1);
    }
}
