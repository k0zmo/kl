#include "kl/binary_rw.hpp"
#include "kl/binary_rw/endian.hpp"
#include "kl/binary_rw/map.hpp"
#include "kl/binary_rw/optional.hpp"
#include "kl/binary_rw/set.hpp"
#include "kl/binary_rw/string.hpp"
#include "kl/binary_rw/variant.hpp"
#include "kl/binary_rw/vector.hpp"

#include <boost/endian/arithmetic.hpp>
#include <catch2/catch_test_macros.hpp>

#include <gsl/span>
#include <gsl/span_ext> // operator==

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

static constexpr inline std::byte operator""_b(unsigned long long i)
{
    return static_cast<std::byte>(i);
}

static constexpr inline std::byte operator""_b(char c)
{
    return static_cast<std::byte>(c);
}

TEST_CASE("binary_reader")
{
    using namespace kl;

    SECTION("empty span")
    {
        binary_reader r{gsl::span<const std::byte>{}};

        REQUIRE(r.left() == 0);
        REQUIRE(r.pos() == 0);
        REQUIRE(r.empty());
        REQUIRE(!r.err());

        SECTION("try peek")
        {
            unsigned char c;
            REQUIRE(!r.peek(c));
            REQUIRE(!r.err());
        }

        SECTION("try read")
        {
            unsigned char c;
            REQUIRE(!r.read(c));
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
        std::array<std::byte, 4> buf = {1_b, 2_b, 3_b, 4_b};
        binary_reader r{buf};

        REQUIRE(r.left() == 4);
        REQUIRE(r.peek<std::byte>() == 1_b);
        REQUIRE(r.peek<boost::endian::big_uint16_t>() == 0x0102U);
        REQUIRE(r.peek<boost::endian::big_uint32_t>() == 0x01020304U);
        REQUIRE(r.read<std::byte>() == 1_b);
        REQUIRE(r.pos() == 1);
        REQUIRE(!r.err());

        REQUIRE(r.peek<std::byte>() == 2_b);
        REQUIRE(r.peek<boost::endian::big_uint16_t>() == 0x0203U);
        REQUIRE(r.left() == 3);
        REQUIRE(r.read<boost::endian::big_uint24_t>() == 0x020304U);
        REQUIRE(r.left() == 0);
        REQUIRE(r.pos() == 4);
    }
}

TEST_CASE("binary_reader - map")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const std::byte>{}};
        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 4 + 4 + 4 + 1 + 1> buf = {
            3_b, 0_b, 0_b, 0_b, 0_b, 0_b,  0_b,
            0_b, 1_b, 0_b, 0_b, 0_b, 64_b, 0_b};
        binary_reader r{buf};

        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("2 elems")
    {
        std::array<std::byte, 4 + 1 + 4 + 1 + 4> buf = {
            2_b, 0_b, 0_b, 0_b,  2_b, 100_b, 0_b,
            0_b, 0_b, 1_b, 10_b, 0_b, 0_b,   0_b};
        binary_reader r{buf};

        auto ret = r.read<std::map<std::byte, int>>();
        REQUIRE(ret.size() == 2);
        REQUIRE(ret[1_b] == 10);
        REQUIRE(ret[2_b] == 100);
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - set")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const std::byte>{}};
        auto ret = r.read<std::set<int>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 4 + 2> buf = {3_b, 0_b, 0_b, 0_b, 1_b, 2_b};
        binary_reader r{buf};

        auto ret = r.read<std::set<std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("3 elems")
    {
        std::array<std::byte, 4 + 3> buf = {3_b, 0_b, 0_b, 0_b, 1_b, 2_b, 3_b};
        binary_reader r{buf};

        auto ret = r.read<std::set<std::byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret.find(1_b) != ret.end());
        REQUIRE(ret.find(2_b) != ret.end());
        REQUIRE(ret.find(3_b) != ret.end());
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - optional")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const std::byte>{}};
        auto ret = r.read<std::optional<int>>();
        REQUIRE(r.err());
    }

    SECTION("nullopt")
    {
        std::array<std::byte, 1> buf = {0_b};
        binary_reader r{buf};

        auto ret = r.read<std::optional<int>>();
        REQUIRE(!ret);
        REQUIRE(!r.err());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 2> buf = {1_b, 0_b};
        binary_reader r{buf};

        auto ret = r.read<std::optional<int>>();
        REQUIRE(r.err());
    }

    SECTION("ok")
    {

        std::array<std::byte, 1 + 4 + 1> buf = {1_b, 1_b, 0_b, 0_b, 0_b, '!'_b};
        binary_reader r{buf};

        auto ret = r.read<std::optional<std::string>>();
        REQUIRE(!r.err());
        REQUIRE(ret);
        REQUIRE(*ret == "!");
    }
}

TEST_CASE("binary_reader - string")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const std::byte>{}};
        auto ret = r.read<std::string>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 4 + 12> buf = {13_b,  0_b,   0_b,   0_b,   'H'_b,
                                             'e'_b, 'l'_b, 'l'_b, 'o'_b, ','_b,
                                             ' '_b, 'w'_b, 'o'_b, 'r'_b, 'l'_b};
        binary_reader r{buf};

        std::string ret;
        REQUIRE(!r.read(ret));
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("ok")
    {
        std::array<std::byte, 4 + 13> buf = {
            13_b,  0_b,   0_b,   0_b,   'H'_b, 'e'_b, 'l'_b, 'l'_b, 'o'_b,
            ','_b, ' '_b, 'w'_b, 'o'_b, 'r'_b, 'l'_b, 'd'_b, '!'_b};
        binary_reader r{buf};

        auto ret = r.read<std::string>();
        REQUIRE(ret == "Hello, world!");
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - variant")
{
    using namespace kl;
    using variant = std::variant<boost::endian::little_int32_t, std::string>;

    auto foo = []() {
        auto ret = variant{};
        return ret;
    };

    auto q = foo();

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const std::byte>{}};
        auto ret = r.read<variant>();
        REQUIRE(r.err());
    }

    SECTION("invalid 'which'")
    {
        std::array<std::byte, 1> buf = {3_b};
        binary_reader r{buf};
        auto ret = r.read<variant>();
        REQUIRE(r.err());
    }

    SECTION("read as int")
    {
        std::array<std::byte, 1 + 4> buf = {0_b, 19_b, 0_b, 0_b, 0_b};
        binary_reader r{buf};

        variant var;
        r.read(var);
        REQUIRE(var.index() == 0);
        REQUIRE(std::get<boost::endian::little_int32_t>(var) == 19);
        REQUIRE(r.empty());
    }

    SECTION("read as int, buffer too short")
    {
        std::array<std::byte, 1 + 2> buf = {0_b, 19_b, 0_b};
        binary_reader r{buf};

        variant var;
        REQUIRE(!r.read(var));
    }

    SECTION("read as string")
    {
        std::array<std::byte, 1 + 4 + 19> buf = {
            1_b,   19_b,  0_b,   0_b,   0_b,   'H'_b, 'e'_b, 'l'_b,
            'l'_b, 'o'_b, ','_b, ' '_b, 'w'_b, 'o'_b, 'r'_b, 'l'_b,
            'd'_b, '!'_b, ' '_b, ' '_b, ' '_b, ' '_b, ' '_b, ' '_b};
        binary_reader r{buf};

        auto ret = r.read<variant>();
        REQUIRE(ret.index() == 1);
        REQUIRE(std::get<std::string>(ret) == "Hello, world!      ");
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - vector")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const std::byte>{}};
        auto ret = r.read<std::vector<int>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 4 + 2> buf = {3_b, 0_b, 0_b, 0_b, 1_b, 2_b};
        binary_reader r{buf};

        auto ret = r.read<std::vector<std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short - trivial type")
    {
        std::array<std::byte, 4 + 2> buf = {3_b, 0_b, 0_b, 0_b, 1_b, 2_b};
        binary_reader r{buf};
        auto ret = r.read<std::vector<std::byte>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("3 elems")
    {
        std::array<std::byte, 4 + 3 + 1> buf = {3_b, 0_b, 0_b, 0_b,
                                                1_b, 2_b, 3_b, 4_b};
        binary_reader r{buf};

        auto ret = r.read<std::vector<std::byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret[0] == 1_b);
        REQUIRE(ret[1] == 2_b);
        REQUIRE(ret[2] == 3_b);
        REQUIRE(!r.err());
        REQUIRE(r.left() == 1);
    }

    SECTION("2 elems - short")
    {
        std::array<std::byte, 4 + 4> buf = {2_b, 0_b, 0_b, 0_b,
                                            1_b, 0_b, 2_b, 0_b};
        binary_reader r{buf};

        auto ret = r.read<std::vector<boost::endian::little_int16_t>>();
        REQUIRE(ret.size() == 2);
        REQUIRE(ret[0] == 1);
        REQUIRE(ret[1] == 2);
        REQUIRE(!r.err());
        REQUIRE(r.left() == 0);
    }
}

TEST_CASE("binary_writer")
{
    using namespace kl;

    SECTION("empty span")
    {
        binary_writer w{gsl::span<std::byte>{}};

        REQUIRE(w.left() == 0);
        REQUIRE(w.pos() == 0);
        REQUIRE(w.empty());
        REQUIRE(!w.err());

        SECTION("try to write 1 byte")
        {
            unsigned char c = 1;
            w << c;
            REQUIRE(w.err());
        }

        SECTION("notify error")
        {
            w.notify_error();
            REQUIRE(w.err());
        }

        SECTION("skip by negative")
        {
            w.skip(-1);
            REQUIRE(w.err());
        }

        SECTION("skip by positive")
        {
            w.skip(1);
            REQUIRE(w.err());
        }
    }

    SECTION("span with 4 bytes")
    {
        std::array<std::byte, 4> buf{};
        binary_writer w{buf};

        REQUIRE(w.left() == 4);
        REQUIRE(!w.empty());
        REQUIRE(!w.err());

        w.skip(1);
        w.skip(-1);
        REQUIRE(w.left() == 4);

        SECTION("write single byte(s)")
        {
            w << static_cast<std::byte>(2);
            REQUIRE(w.left() == 3);
            REQUIRE(!w.empty());
            REQUIRE(!w.err());
            REQUIRE(buf[0] == 2_b);
            REQUIRE(buf[1] == 0_b);
            REQUIRE(buf[2] == 0_b);
            REQUIRE(buf[3] == 0_b);

            w << static_cast<std::byte>(5) << static_cast<std::byte>(66);
            REQUIRE(w.left() == 1);
            REQUIRE(!w.empty());
            REQUIRE(!w.err());
            REQUIRE(buf[0] == 2_b);
            REQUIRE(buf[1] == 5_b);
            REQUIRE(buf[2] == 66_b);
            REQUIRE(buf[3] == 0_b);

            w << static_cast<std::byte>(255);
            REQUIRE(buf[3] == 255_b);
            REQUIRE(w.left() == 0);
            REQUIRE(w.empty());
            REQUIRE(!w.err());

            w << static_cast<std::byte>(255);
            REQUIRE(w.empty());
            REQUIRE(w.err());
        }

        SECTION("write int")
        {
            w << boost::endian::little_uint32_t{0x01020304};
            REQUIRE(w.empty());
            REQUIRE(buf[0] == 0x04_b);
            REQUIRE(buf[1] == 0x03_b);
            REQUIRE(buf[2] == 0x02_b);
            REQUIRE(buf[3] == 0x01_b);
        }
    }
}

TEST_CASE("binary_writer - string")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<std::byte>{}};
        w << std::string{"Test"};
        REQUIRE(w.err());
    }

    SECTION("buffer too small")
    {
        std::array<std::byte, 7> buf{};
        binary_writer w{buf};
        w << std::string{"Test"};
        REQUIRE(w.err());

        // Check that only size was written
        binary_reader r{buf};
        REQUIRE(4 == r.read<std::uint32_t>());
        REQUIRE(r.left() == 3);
    }

    SECTION("write string")
    {
        std::array<std::byte, 9> buf{};
        binary_writer w{buf};
        w << std::string{"Test"};
        REQUIRE(w.left() == 1);
        REQUIRE(!w.err());

        binary_reader r{buf};
        REQUIRE("Test" == r.read<std::string>());
        REQUIRE(r.left() == 1);
    }
}

TEST_CASE("binary_writer - map")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<std::byte>{}};
        w << std::map<int, std::string>{};
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 18> buf{};
        binary_writer w{buf};
        w << std::map<int, std::string>{{0, "Test"}, {1, "ZXC"}};

        REQUIRE(w.err());
        REQUIRE(w.pos() == 16);
    }

    SECTION("2 elems")
    {
        std::array<std::byte, 27> buf{};
        binary_writer w{buf};
        w << std::map<int, std::string>{{100, "Test"}, {200, "ZXC"}};

        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(ret.size() == 2);
        REQUIRE(ret[100] == "Test");
        REQUIRE(ret[200] == "ZXC");
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_writer - set")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<std::byte>{}};
        w << std::set<int>{};
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 10> buf{};
        binary_writer w{buf};

        w << std::set<std::string>{"ABC", "ZXC"};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 8);
    }

    SECTION("3 elems")
    {
        std::array<std::byte, 7> buf{};
        binary_writer w{buf};

        w << std::set<std::byte>{1_b, 2_b, 3_b};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<std::set<std::byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret.find(1_b) != ret.end());
        REQUIRE(ret.find(2_b) != ret.end());
        REQUIRE(ret.find(3_b) != ret.end());
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_writer - optional")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<std::byte>{}};
        w << std::optional<int>{44};
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 2> buf{};
        binary_writer w{buf};

        w << std::optional<int>{32};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 1);
        REQUIRE(buf[0] == 1_b);
    }

    SECTION("nullopt")
    {
        std::array<std::byte, 1> buf{};
        binary_writer w{buf};

        w << std::optional<int>{};
        REQUIRE(!w.err());
        REQUIRE(w.empty());
        REQUIRE(buf[0] == 0_b);
    }

    SECTION("ok")
    {
        std::array<std::byte, 7> buf{};
        binary_writer w{buf};

        w << std::optional<std::string>{"!?"};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        REQUIRE("!?" == r.read<std::optional<std::string>>().value_or("ZZ"));
        REQUIRE(r.left() == 0);
    }
}

TEST_CASE("binary_writer - variant")
{
    using namespace kl;
    using variant = std::variant<boost::endian::little_int32_t, std::string>;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<std::byte>{}};
        w << variant{1};
        REQUIRE(w.err());
    }

    SECTION("try to write int, buffer too short")
    {
        std::array<std::byte, 3> buf{1_b, 0_b, 0_b};
        binary_writer w{buf};

        w << variant{};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 1);
        REQUIRE(buf[0] == 0_b);
    }

    SECTION("write ints")
    {
        std::array<std::byte, 10> buf{};
        binary_writer w{buf};

        variant var{9001};
        w << var;
        w << variant{9002};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        r.read(var);
        REQUIRE(var.index() == 0);
        REQUIRE(std::get<boost::endian::little_int32_t>(var) == 9001);
        r.read(var);
        REQUIRE(var.index() == 0);
        REQUIRE(std::get<boost::endian::little_int32_t>(var) == 9002);
        REQUIRE(r.empty());
        REQUIRE(!r.err());
    }

    SECTION("write string")
    {
        std::array<std::byte, 24> buf{};
        binary_writer w{buf};

        w << variant{"Hello, world!      "};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<variant>();
        REQUIRE(ret.index() == 1);
        REQUIRE(std::get<std::string>(ret) == "Hello, world!      ");
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_writer - vector")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<std::byte>{}};
        w << std::vector<int>();
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<std::byte, 10> buf{};
        binary_writer w{buf};

        w << std::vector<std::string>{"test", "zxc"};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 8);
    }

    SECTION("3 elems")
    {
        std::array<std::byte, 10> buf{};
        binary_writer w{buf};

        w << std::vector<boost::endian::little_int16_t>{100, 200, 300};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<std::vector<boost::endian::little_int16_t>>();
        REQUIRE(r.empty());
        REQUIRE(!r.err());

        REQUIRE(ret.size() == 3);
        REQUIRE(ret[0] == 100);
        REQUIRE(ret[1] == 200);
        REQUIRE(ret[2] == 300);
    }

    SECTION("2 elems")
    {
        std::array<std::byte, 19> buf{};
        binary_writer w{buf};

        w << std::vector<std::string>{"test", "zxc"};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<std::vector<std::string>>();
        REQUIRE(r.empty());
        REQUIRE(!r.err());
        REQUIRE(ret.size() == 2);
        REQUIRE(ret[0] == "test");
        REQUIRE(ret[1] == "zxc");
    }
}

struct user_defined_type
{
    float vec[4];
    int i;
    float f;
};

void read_binary(kl::binary_reader& r, user_defined_type& value)
{
    r >> gsl::span{value.vec};
    r >> value.i;
    r >> value.f;
}

void write_binary(kl::binary_writer& w, const user_defined_type& value)
{
    w << gsl::span{value.vec};
    w << value.i;
    w << value.f;
}

TEST_CASE("binary_reader/writer - user defined type")
{
    std::array<char, 52> buf{};
    kl::binary_writer w{gsl::span<char>{buf}};

    w << std::vector<user_defined_type>{
        {{3.14f, 1.0f, 2.72f, 9000.0f}, 2, 0.0f},
        {{0.0f, 0.0f, 2.0f, 3.0f}, 55, 10000.0f}};

    REQUIRE(w.empty());
    REQUIRE(!w.err());

    kl::binary_reader r{gsl::span<char>{buf}};
    std::vector<user_defined_type> v;
    r >> v;
    REQUIRE(r.empty());
    REQUIRE(!r.err());

    REQUIRE(v.size() == 2);
    REQUIRE(v[0].vec[0] == 3.14f);
    REQUIRE(v[0].vec[1] == 1.0f);
    REQUIRE(v[0].vec[2] == 2.72f);
    REQUIRE(v[0].vec[3] == 9000.0f);
    REQUIRE(v[0].i == 2);
    REQUIRE(v[0].f == 0.0f);
    REQUIRE(v[1].vec[0] == 0);
    REQUIRE(v[1].vec[1] == 0);
    REQUIRE(v[1].vec[2] == 2.0f);
    REQUIRE(v[1].vec[3] == 3);
    REQUIRE(v[1].i == 55);
    REQUIRE(v[1].f == 10000.0f);
}
