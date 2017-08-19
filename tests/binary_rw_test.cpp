#include "kl/binary_rw.hpp"
#include "kl/binary_rw/endian.hpp"
#include "kl/binary_rw/map.hpp"
#include "kl/binary_rw/optional.hpp"
#include "kl/binary_rw/set.hpp"
#include "kl/binary_rw/string.hpp"
#include "kl/binary_rw/variant.hpp"
#include "kl/binary_rw/vector.hpp"

#include <catch/catch.hpp>
#include <boost/optional/optional_io.hpp>
#include <gsl/string_span>

#include <cstring>

TEST_CASE("binary_reader")
{
    using namespace kl;

    SECTION("empty span")
    {
        binary_reader r{gsl::span<const byte>()};

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
        std::array<byte, 4> buf = {1, 2, 3, 4};
        binary_reader r{buf};

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
        binary_reader r(gsl::ensure_z(str));

        REQUIRE(r.left() == 12);
        REQUIRE(gsl::as_bytes(r.span(r.left())) ==
                gsl::as_bytes(gsl::ensure_z(str)));
        REQUIRE(r.pos() == 12);
        REQUIRE(r.left() == 0);

        r.skip(-1);
        REQUIRE(r.read<char>() == '!');
        REQUIRE(r.left() == 0);
    }
}

TEST_CASE("binary_reader - map")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 4 + 4 + 1 + 1> buf = {3, 0, 0, 0, 0, 0,   0,
                                                   0, 1, 0, 0, 0, '@', 0};
        binary_reader r{buf};

        auto ret = r.read<std::map<int, std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("2 elems")
    {
        std::array<byte, 4 + 1 + 4 + 1 + 4> buf = {2, 0, 0, 0,  2, 100, 0,
                                                   0, 0, 1, 10, 0, 0,   0};
        binary_reader r{buf};

        auto ret = r.read<std::map<byte, int>>();
        REQUIRE(ret.size() == 2);
        REQUIRE(ret[1] == 10);
        REQUIRE(ret[2] == 100);
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - set")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::set<int>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 2> buf = {3, 0, 0, 0, 1, 2};
        binary_reader r{buf};

        auto ret = r.read<std::set<std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("3 elems")
    {
        std::array<byte, 4 + 3> buf = {3, 0, 0, 0, 1, 2, 3};
        binary_reader r{buf};

        auto ret = r.read<std::set<byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret.find(1) != ret.end());
        REQUIRE(ret.find(2) != ret.end());
        REQUIRE(ret.find(3) != ret.end());
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - optional")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const byte>{}};
        auto ret = r.read<boost::optional<int>>();
        REQUIRE(r.err());
    }

    SECTION("nullopt")
    {
        std::array<byte, 1> buf = {0};
        binary_reader r{buf};

        auto ret = r.read<boost::optional<int>>();
        REQUIRE(!ret);
        REQUIRE(!r.err());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 2> buf = {1, 0};
        binary_reader r{buf};

        auto ret = r.read<boost::optional<int>>();
        REQUIRE(r.err());
    }

    SECTION("ok")
    {
        std::array<byte, 1 + 4 + 1> buf = {1, 1, 0, 0, 0, '!'};
        binary_reader r{buf};

        auto ret = r.read<boost::optional<std::string>>();
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
        binary_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::string>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 12> buf = {13,  0,   0,   0,   'H', 'e', 'l', 'l',
                                        'o', ',', ' ', 'w', 'o', 'r', 'l'};
        binary_reader r{buf};

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
    using variant = boost::variant<boost::endian::little_int32_t, std::string>;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const byte>{}};
        auto ret = r.read<variant>();
        REQUIRE(r.err());
    }

    SECTION("invalid 'which'")
    {
        std::array<byte, 1> buf = {3};
        binary_reader r{buf};
        auto ret = r.read<variant>();
        REQUIRE(r.err());
    }

    SECTION("read as int")
    {
        std::array<byte, 1 + 4> buf = {0, 19, 0, 0, 0};
        binary_reader r{buf};

        variant var;
        r.read(var);
        REQUIRE(var.which() == 0);
        REQUIRE(boost::get<boost::endian::little_int32_t>(var) == 19);
        REQUIRE(r.empty());
    }

    SECTION("read as int, buffer too short")
    {
        std::array<byte, 1 + 2> buf = {0, 19, 0};
        binary_reader r{buf};

        variant var;
        REQUIRE(!r.read(var));
    }

    SECTION("read as string")
    {
        std::array<byte, 1 + 4 + 19> buf = {
            1,   19,  0,   0,   0,   'H', 'e', 'l', 'l', 'o', ',', ' ',
            'w', 'o', 'r', 'l', 'd', '!', ' ', ' ', ' ', ' ', ' ', ' '};
        binary_reader r{buf};

        auto ret = r.read<variant>();
        REQUIRE(ret.which() == 1);
        REQUIRE(boost::get<std::string>(ret) == "Hello, world!      ");
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_reader - vector")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_reader r{gsl::span<const byte>{}};
        auto ret = r.read<std::vector<int>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 4 + 2> buf = {3, 0, 0, 0, 1, 2};
        binary_reader r{buf};

        auto ret = r.read<std::vector<std::string>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("buffer too short - trivial type")
    {
        std::array<byte, 4 + 2> buf = {3, 0, 0, 0, 1, 2};
        binary_reader r{buf};
        auto ret = r.read<std::vector<byte>>();
        REQUIRE(r.err());
        REQUIRE(ret.empty());
    }

    SECTION("3 elems")
    {
        std::array<byte, 4 + 3 + 1> buf = {3, 0, 0, 0, 1, 2, 3, 4};
        binary_reader r{buf};

        auto ret = r.read<std::vector<byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret[0] == 1);
        REQUIRE(ret[1] == 2);
        REQUIRE(ret[2] == 3);
        REQUIRE(!r.err());
        REQUIRE(r.left() == 1);
    }

    SECTION("2 elems - short")
    {
        std::array<byte, 4 + 4> buf = {2, 0, 0, 0, 1, 0, 2, 0};
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
        binary_writer w{gsl::span<byte>()};

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
        std::array<byte, 4> buf{};
        binary_writer w{buf};

        REQUIRE(w.left() == 4);
        REQUIRE(!w.empty());
        REQUIRE(!w.err());

        w.skip(1);
        w.skip(-1);
        REQUIRE(w.left() == 4);

        SECTION("write single byte(s)")
        {
            w << static_cast<kl::byte>(2);
            REQUIRE(w.left() == 3);
            REQUIRE(!w.empty());
            REQUIRE(!w.err());
            REQUIRE(buf[0] == 2);
            REQUIRE(buf[1] == 0);
            REQUIRE(buf[2] == 0);
            REQUIRE(buf[3] == 0);

            w << static_cast<kl::byte>(5) << static_cast<kl::byte>(66);
            REQUIRE(w.left() == 1);
            REQUIRE(!w.empty());
            REQUIRE(!w.err());
            REQUIRE(buf[0] == 2);
            REQUIRE(buf[1] == 5);
            REQUIRE(buf[2] == 66);
            REQUIRE(buf[3] == 0);

            w << static_cast<kl::byte>(255);
            REQUIRE(buf[3] == 255);
            REQUIRE(w.left() == 0);
            REQUIRE(w.empty());
            REQUIRE(!w.err());

            w << static_cast<kl::byte>(255);
            REQUIRE(w.empty());
            REQUIRE(w.err());
        }

        SECTION("write int")
        {
            w << boost::endian::little_uint32_t{0x01020304};
            REQUIRE(w.empty());
            REQUIRE(buf[0] == 0x04);
            REQUIRE(buf[1] == 0x03);
            REQUIRE(buf[2] == 0x02);
            REQUIRE(buf[3] == 0x01);
        }
    }

    SECTION("write cstring as span")
    {
        const char* str = "Hello, world!";
        std::array<byte, 13> buf{};
        binary_writer w{buf};

        w << gsl::ensure_z(str);
        REQUIRE(!w.err());
        REQUIRE(w.empty());
        REQUIRE(gsl::as_bytes(gsl::ensure_z(str)) ==
                gsl::as_bytes(gsl::make_span(buf)));
    }
}

TEST_CASE("binary_writer - string")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<byte>{}};
        w << std::string{"Test"};
        REQUIRE(w.err());
    }

    SECTION("buffer too small")
    {
        std::array<kl::byte, 7> buf{};
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
        std::array<kl::byte, 9> buf{};
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
        binary_writer w{gsl::span<byte>{}};
        w << std::map<int, std::string>{};
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 18> buf{};
        binary_writer w{buf};
        w << std::map<int, std::string>{{0, "Test"}, {1, "ZXC"}};

        REQUIRE(w.err());
        REQUIRE(w.pos() == 16);
    }

    SECTION("2 elems")
    {
        std::array<byte, 27> buf{};
        binary_writer w{buf};
        w << std::map<int, std::string>{ {100, "Test"}, {200, "ZXC"}};

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
        binary_writer w{gsl::span<byte>{}};
        w << std::set<int>{};
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 10> buf{};
        binary_writer w{buf};

        w << std::set<std::string>{"ABC", "ZXC"};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 8);
    }

    SECTION("3 elems")
    {
        std::array<byte, 7> buf{};
        binary_writer w{buf};

        w << std::set<byte>{1, 2, 3};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<std::set<byte>>();
        REQUIRE(ret.size() == 3);
        REQUIRE(ret.find(1) != ret.end());
        REQUIRE(ret.find(2) != ret.end());
        REQUIRE(ret.find(3) != ret.end());
        REQUIRE(!r.err());
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_writer - optional")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<byte>{}};
        w << boost::optional<int>{44};
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 2> buf{};
        binary_writer w{buf};

        w << boost::optional<int>{32};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 1);
        REQUIRE(buf[0] == 1);
    }

    SECTION("nullopt")
    {
        std::array<byte, 1> buf{};
        binary_writer w{buf};

        w << boost::optional<int>{};
        REQUIRE(!w.err());
        REQUIRE(w.empty());
        REQUIRE(buf[0] == 0);
    }

    SECTION("ok")
    {
        std::array<byte, 7> buf{};
        binary_writer w{buf};

        w << boost::optional<std::string>{"!?"};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        REQUIRE("!?" ==
                r.read<boost::optional<std::string>>().get_value_or("ZZ"));
        REQUIRE(r.left() == 0);
    }
}

TEST_CASE("binary_writer - variant")
{
    using namespace kl;
    using variant = boost::variant<boost::endian::little_int32_t, std::string>;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<byte>{}};
        w << variant{1};
        REQUIRE(w.err());
    }

    SECTION("try to write int, buffer too short")
    {
        std::array<byte, 3> buf{1,0,0};
        binary_writer w{buf};

        w << variant{};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 1);
        REQUIRE(buf[0] == 0);
    }

    SECTION("write ints")
    {
        std::array<byte, 10> buf{};
        binary_writer w{buf};

        variant var{9001};
        w << var;
        w << variant{9002};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        r.read(var);
        REQUIRE(var.which() == 0);
        REQUIRE(boost::get<boost::endian::little_int32_t>(var) == 9001);
        r.read(var);
        REQUIRE(var.which() == 0);
        REQUIRE(boost::get<boost::endian::little_int32_t>(var) == 9002);
        REQUIRE(r.empty());
        REQUIRE(!r.err());
    }

    SECTION("write string")
    {
        std::array<byte, 24> buf{};
        binary_writer w{buf};

        w << variant{"Hello, world!      "};
        REQUIRE(!w.err());
        REQUIRE(w.empty());

        binary_reader r{buf};
        auto ret = r.read<variant>();
        REQUIRE(ret.which() == 1);
        REQUIRE(boost::get<std::string>(ret) == "Hello, world!      ");
        REQUIRE(r.empty());
    }
}

TEST_CASE("binary_writer - vector")
{
    using namespace kl;

    SECTION("empty buffer")
    {
        binary_writer w{gsl::span<byte>{}};
        w << std::vector<int>();
        REQUIRE(w.err());
    }

    SECTION("buffer too short")
    {
        std::array<byte, 10> buf{};
        binary_writer w{buf};

        w << std::vector<std::string>{"test", "zxc"};
        REQUIRE(w.err());
        REQUIRE(w.pos() == 8);
    }

    SECTION("3 elems")
    {
        std::array<byte, 10> buf{};
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
        std::array<byte, 19> buf{};
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
    r >> gsl::make_span(value.vec);
    r >> value.i;
    r >> value.f;
}

void write_binary(kl::binary_writer& w, const user_defined_type& value)
{
    w << gsl::make_span(value.vec);
    w << value.i;
    w << value.f;
}

TEST_CASE("binary_reader/writer - user defined type")
{
    std::array<char, 52> buf{};
    kl::binary_writer w{gsl::make_span(buf)};

    w << std::vector<user_defined_type>{
        {{3.14f, 1.0f, 2.72f, 9000.0f}, 2, 0.0f},
        {{0.0f, 0.0f, 2.0f, 3.0f}, 55, 10000.0f}};

    REQUIRE(w.empty());
    REQUIRE(!w.err());

    kl::binary_reader r{gsl::make_span(buf)};
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
