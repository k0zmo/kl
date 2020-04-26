#include "kl/enum_set.hpp"

#include <catch2/catch.hpp>

namespace {

enum class type_qualifier
{
    none = 0,
    const_ = (1 << 0),
    restrict_ = (1 << 1),
    volatile_ = (1 << 2)
};
using type_qualifier_flags = kl::enum_set<type_qualifier>;

enum device_type : std::uint32_t
{
    default_ = (1 << 0),
    cpu = (1 << 1),
    gpu = (1 << 2),
    accelerator = (1 << 3),
    custom = (1 << 4),
    all = 0xffffffff
};
using device_flags = kl::enum_set<device_type>;

enum class memory_type : unsigned short
{
    unspecified = 0,
    private_ = 0b1,
    local_ = 0b10,
    const_ = 0b100,
    global_ = 0b1000
};
using memory_flags = kl::enum_set<memory_type>;
} // namespace

TEST_CASE("enum_set - unscoped enum")
{
    SECTION("default state")
    {
        device_flags f;
        CHECK(f.underlying_value() == 0);
        CHECK_FALSE(f.test(default_));
        CHECK_FALSE(f.test(cpu));
        CHECK_FALSE(f.test(gpu));
        CHECK_FALSE(f.test(accelerator));
        CHECK_FALSE(f.test(custom));
        CHECK_FALSE(f.test(all));
        CHECK_FALSE(f.has_all(all));
        CHECK_FALSE(f.has_any(all));
    }

    SECTION("or")
    {
        device_flags f;
        CHECK((f | gpu).test(gpu));
        CHECK((gpu | f).test(gpu));
        CHECK((f | gpu).has_any(gpu));
        CHECK((gpu | f).has_all(gpu));

        f |= gpu;
        CHECK(f.test(gpu));
        CHECK(f.has_any(gpu));
        CHECK(f.has_any(kl::enum_set{cpu} | gpu));
        CHECK(f.has_all(gpu));
        CHECK_FALSE(f.has_all(kl::enum_set{cpu} | gpu));

        f |= cpu;
        CHECK(f.test(cpu));
        CHECK(f.has_any(cpu));
        CHECK(f.has_all(cpu));

        CHECK(f.has_any(kl::enum_set{cpu} | gpu));
        CHECK(f.has_all(kl::enum_set{cpu} | gpu));
        CHECK(f.has_any(kl::enum_set{cpu} | gpu | accelerator));
        CHECK_FALSE(f.has_all(kl::enum_set{cpu} | gpu | accelerator));
    }

    SECTION("and")
    {
        device_flags f = kl::enum_set{gpu} | cpu;
        CHECK((f & cpu).test(cpu));
        CHECK((cpu & f & cpu).test(cpu));
        CHECK_FALSE((f & cpu).test(gpu));
        CHECK_FALSE((f & accelerator).test(accelerator));
        CHECK(f.has_any(kl::enum_set{cpu} | gpu));
        CHECK(f.has_any(kl::enum_set{cpu} | gpu | accelerator));
        CHECK(f.has_all(kl::enum_set{cpu} | gpu));
        CHECK_FALSE(f.has_all(kl::enum_set{cpu} | gpu | accelerator));
        f &= cpu;
        CHECK(f.underlying_value() == cpu);
    }

    SECTION("xor")
    {
        device_flags f = device_flags{gpu} | cpu;
        CHECK_FALSE((f ^ all).test(gpu));
        CHECK_FALSE((all ^ f).test(cpu));
        CHECK((f ^ all).test(accelerator));

        f ^= all;
        CHECK_FALSE(f.test(gpu));
        CHECK_FALSE(f.test(cpu));
        CHECK(f.test(custom));
    }

    SECTION("not")
    {
        device_flags f;
        auto f2 = device_flags{all};
        CHECK((~f2).underlying_value() == 0);
        CHECK((~f).value() == all);
    }

    SECTION("operators eq/neq/lt")
    {
        CHECK(device_flags{all} == device_flags{all});
        CHECK(device_flags{all} == all);
        CHECK(all == device_flags{all});

        CHECK(device_flags{all} != device_flags{cpu});
        CHECK(device_flags{all} != cpu);
        CHECK(all != device_flags{cpu});

        CHECK(device_flags{cpu} < device_flags{gpu});
        CHECK(device_flags{cpu} < gpu);
        CHECK(cpu < device_flags{gpu});
    }
}

TEST_CASE("enum_set - scoped enum")
{
    SECTION("default state")
    {
        type_qualifier_flags f;
        CHECK(f.underlying_value() == 0);
        CHECK(f.test(type_qualifier::none));
        CHECK_FALSE(f.test(type_qualifier::const_));
        CHECK_FALSE(f.test(type_qualifier::restrict_));
        CHECK_FALSE(f.test(type_qualifier::volatile_));
    }

    SECTION("or")
    {
        type_qualifier_flags f;
        CHECK((f | type_qualifier::const_).test(type_qualifier::const_));
        CHECK((type_qualifier::const_ | f).test(type_qualifier::const_));
        f |= type_qualifier::const_;
        f.test(type_qualifier::const_);
    }

    SECTION("and")
    {
        type_qualifier_flags f =
            kl::enum_set{type_qualifier::const_} | type_qualifier::volatile_;
        CHECK((f & type_qualifier::restrict_).value() == type_qualifier::none);
        CHECK((type_qualifier::const_ & f).value() == type_qualifier::const_);

        f &= type_qualifier::const_;
        CHECK(f.underlying_value() == 1);

        f &= type_qualifier::none;
        CHECK(f.underlying_value() == 0);
    }

    SECTION("xor")
    {
        type_qualifier_flags f;
        CHECK((f ^ type_qualifier::const_ ^ type_qualifier::restrict_)
                  .test(type_qualifier::const_));
        f ^= type_qualifier::restrict_;
        f ^= type_qualifier::volatile_;

        CHECK(f.test(type_qualifier::restrict_));
        CHECK_FALSE(f.test(type_qualifier::const_));
    }

    SECTION("not")
    {
        type_qualifier_flags f;
        CHECK((~f).test(type_qualifier::const_));
        CHECK((~f).test(type_qualifier::restrict_));
        CHECK((~f).test(type_qualifier::volatile_));
    }

    SECTION("operator bool")
    {
        CHECK(kl::enum_set{type_qualifier::const_});
        CHECK_FALSE(kl::enum_set{type_qualifier::none});
    }
}

TEST_CASE("enum_set - short underlying type")
{
    SECTION("default state")
    {
        memory_flags f;
        CHECK(f.underlying_value() == 0);
        CHECK_FALSE(f.test(memory_type::private_));
        CHECK_FALSE(f.test(memory_type::local_));
        CHECK_FALSE(f.test(memory_type::const_));
        CHECK_FALSE(f.test(memory_type::global_));
    }

    SECTION("or")
    {
        memory_flags f;
        CHECK((f | memory_type::const_).test(memory_type::const_));
        CHECK((memory_type::const_ | f).test(memory_type::const_));
        f |= memory_type::const_;
        f.test(memory_type::const_);
    }

    SECTION("and")
    {
        memory_flags f =
            kl::enum_set{memory_type::const_} | memory_type::local_;
        CHECK((f & memory_type::private_).value() == memory_type::unspecified);
        CHECK((memory_type::const_ & f).value() == memory_type::const_);

        f &= memory_type::const_;
        CHECK(f.underlying_value() == 4);

        f &= memory_type::unspecified;
        CHECK(f.underlying_value() == 0);
    }

    SECTION("xor")
    {
        memory_flags f;
        CHECK((f ^ memory_type::const_ ^ memory_type::private_)
                  .test(memory_type::const_));
        f ^= memory_type::private_;
        f ^= memory_type::local_;

        CHECK(f.test(memory_type::private_));
        CHECK_FALSE(f.test(memory_type::const_));
    }

    SECTION("not")
    {
        memory_flags f;
        CHECK((~f).test(memory_type::const_));
        CHECK((~f).test(memory_type::private_));
        CHECK((~f).test(memory_type::local_));
        CHECK((~f).test(memory_type::global_));
    }
}
