#include "kl/enum_flags.hpp"

#include <catch2/catch.hpp>

namespace {

enum class type_qualifier
{
    none = 0,
    const_ = (1 << 0),
    restrict_ = (1 << 1),
    volatile_ = (1 << 2)
};
using type_qualifier_flags = kl::enum_flags<type_qualifier>;

enum device_type : std::uint32_t
{
    default_ = (1 << 0),
    cpu = (1 << 1),
    gpu = (1 << 2),
    accelerator = (1 << 3),
    custom = (1 << 4),
    all = 0xffffffff
};
using device_flags = kl::enum_flags<device_type>;

enum class memory_type : unsigned short
{
    unspecified = 0,
    private_ = 0b1,
    local_ = 0b10,
    const_ = 0b100,
    global_ = 0b1000
};
using memory_flags = kl::enum_flags<memory_type>;
} // namespace

TEST_CASE("enum_flags")
{
    SECTION("unscoped enum")
    {
        device_flags flags;
        REQUIRE(flags.underlying_value() == 0);
        REQUIRE_FALSE(flags.test(default_));
        REQUIRE_FALSE(flags.test(cpu));
        REQUIRE_FALSE(flags.test(gpu));
        REQUIRE_FALSE(flags.test(accelerator));
        REQUIRE_FALSE(flags.test(custom));
        REQUIRE_FALSE(flags.test(all));

        SECTION("or")
        {
            REQUIRE((flags | gpu).test(gpu));
            REQUIRE((gpu | flags).test(gpu));
            flags |= gpu;
            flags.test(gpu);
        }

        SECTION("and")
        {
            auto f = kl::make_flags(gpu) | cpu;
            REQUIRE((f & cpu).test(cpu));
            REQUIRE((cpu & f & cpu).test(cpu));
            REQUIRE_FALSE((f & cpu).test(gpu));
            REQUIRE_FALSE((f & accelerator).test(accelerator));
            f &= cpu;
            REQUIRE(f.underlying_value() == cpu);
        }

        SECTION("xor")
        {
            auto f = device_flags{gpu} | cpu;
            REQUIRE_FALSE((f ^ all).test(gpu));
            REQUIRE_FALSE((all ^ f).test(cpu));
            REQUIRE((f ^ all).test(accelerator));

            f ^= all;
            REQUIRE_FALSE(f.test(gpu));
            REQUIRE_FALSE(f.test(cpu));
            REQUIRE(f.test(custom));
        }

        SECTION("not")
        {
            auto f = device_flags{all};
            REQUIRE((~f).underlying_value() == 0);
            REQUIRE((~flags).value() == all);
        }

        SECTION("operators eq/neq/lt")
        {
            REQUIRE(device_flags{all} == device_flags{all});
            REQUIRE(device_flags{all} == all);
            REQUIRE(all == device_flags{all});

            REQUIRE(device_flags{all} != device_flags{cpu});
            REQUIRE(device_flags{all} != cpu);
            REQUIRE(all != device_flags{cpu});

            REQUIRE(device_flags{cpu} < device_flags{gpu});
            REQUIRE(device_flags{cpu} < gpu);
            REQUIRE(cpu < device_flags{gpu});
        }
    }

    SECTION("scoped enum")
    {
        type_qualifier_flags flags;
        REQUIRE(flags.underlying_value() == 0);
        REQUIRE(flags.test(type_qualifier::none));
        REQUIRE_FALSE(flags.test(type_qualifier::const_));
        REQUIRE_FALSE(flags.test(type_qualifier::restrict_));
        REQUIRE_FALSE(flags.test(type_qualifier::volatile_));

        SECTION("or")
        {
            REQUIRE(
                (flags | type_qualifier::const_).test(type_qualifier::const_));
            REQUIRE(
                (type_qualifier::const_ | flags).test(type_qualifier::const_));
            flags |= type_qualifier::const_;
            flags.test(type_qualifier::const_);
        }

        SECTION("and")
        {
            auto f = kl::make_flags(type_qualifier::const_) |
                     type_qualifier::volatile_;
            REQUIRE((f & type_qualifier::restrict_).value() ==
                    type_qualifier::none);
            REQUIRE((type_qualifier::const_ & f).value() ==
                    type_qualifier::const_);

            f &= type_qualifier::const_;
            REQUIRE(f.underlying_value() == 1);

            f &= type_qualifier::none;
            REQUIRE(f.underlying_value() == 0);
        }

        SECTION("xor")
        {
            REQUIRE((flags ^ type_qualifier::const_ ^ type_qualifier::restrict_)
                        .test(type_qualifier::const_));
            flags ^= type_qualifier::restrict_;
            flags ^= type_qualifier::volatile_;

            REQUIRE(flags.test(type_qualifier::restrict_));
            REQUIRE_FALSE(flags.test(type_qualifier::const_));
        }

        SECTION("not")
        {
            REQUIRE((~flags).test(type_qualifier::const_));
            REQUIRE((~flags).test(type_qualifier::restrict_));
            REQUIRE((~flags).test(type_qualifier::volatile_));
        }

        SECTION("operator bool")
        {
            REQUIRE(kl::make_flags(type_qualifier::const_));
            REQUIRE_FALSE(kl::make_flags(type_qualifier::none));
        }
    }

    SECTION("short underlying type")
    {
        memory_flags flags;
        CHECK(flags.underlying_value() == 0);
        CHECK_FALSE(flags.test(memory_type::private_));
        CHECK_FALSE(flags.test(memory_type::local_));
        CHECK_FALSE(flags.test(memory_type::const_));
        CHECK_FALSE(flags.test(memory_type::global_));

        SECTION("or")
        {
            CHECK((flags | memory_type::const_).test(memory_type::const_));
            CHECK((memory_type::const_ | flags).test(memory_type::const_));
            flags |= memory_type::const_;
            flags.test(memory_type::const_);
        }

        SECTION("and")
        {
            auto f = kl::make_flags(memory_type::const_) | memory_type::local_;
            CHECK((f & memory_type::private_).value() ==
                  memory_type::unspecified);
            CHECK((memory_type::const_ & f).value() == memory_type::const_);

            f &= memory_type::const_;
            CHECK(f.underlying_value() == 4);

            f &= memory_type::unspecified;
            CHECK(f.underlying_value() == 0);
        }

        SECTION("xor")
        {
            CHECK((flags ^ memory_type::const_ ^ memory_type::private_)
                      .test(memory_type::const_));
            flags ^= memory_type::private_;
            flags ^= memory_type::local_;

            CHECK(flags.test(memory_type::private_));
            CHECK_FALSE(flags.test(memory_type::const_));
        }

        SECTION("not")
        {
            CHECK((~flags).test(memory_type::const_));
            CHECK((~flags).test(memory_type::private_));
            CHECK((~flags).test(memory_type::local_));
            CHECK((~flags).test(memory_type::global_));
        }
    }
}
