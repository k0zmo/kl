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
    }
}
