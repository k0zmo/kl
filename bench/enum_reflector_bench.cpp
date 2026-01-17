#include <kl/enum_reflector.hpp>
#include <kl/reflect_enum.hpp>

#include <boost/version.hpp>

#if BOOST_VERSION >= 107900
#  define KL_BENCH_BOOST_DESCRIBE
#  include <boost/describe/enum.hpp>
#  include <boost/describe/enumerators.hpp>
#  include <boost/describe/enum_to_string.hpp>
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <limits>
#include <optional>
#include <random>
#include <string>
#include <string_view>

// clang-format off
enum class abcd
{
    a,ab,abc,abcd,abcde,abcdef,abcdefg,abcdefgh,abcdefghi,
    z,zx,zxc,zxcv,zxcvb,zxcvbn,zxcvbnm,zxcvbnma,zxcvbnmas
};
KL_REFLECT_ENUM(
    abcd,
    a,ab,abc,abcd,abcde,abcdef,abcdefg,abcdefgh,abcdefghi,
    z,zx,zxc,zxcv,zxcvb,zxcvbn,zxcvbnm,zxcvbnma,zxcvbnmas
)

#if defined(KL_BENCH_BOOST_DESCRIBE)
BOOST_DESCRIBE_ENUM(abcd,
    a,ab,abc,abcd,abcde,abcdef,abcdefg,abcdefgh,abcdefghi,
    z,zx,zxc,zxcv,zxcvb,zxcvbn,zxcvbnm,zxcvbnma,zxcvbnmas)
// clang-format on

template<class E, class De = boost::describe::describe_enumerators<E>>
static std::optional<E> my_enum_from_string( std::string_view name ) noexcept
{
    std::optional<E> ret;

    boost::mp11::mp_for_each<De>([&](auto D){

        if( !ret && name == D.name )
        {
            ret = D.value;
        }

    });

    return ret;
}
#endif

static int count()
{
    const auto dice = std::random_device{}();
    const auto c = kl::reflect<abcd>().count();
    return dice > std::numeric_limits<std::random_device::result_type>::max() / 2 ? c : c - 1;
}

TEST_CASE("enum reflector bench")
{
    const auto c = count();
    std::string v = kl::reflect<abcd>().to_string(abcd(c - 1));

    BENCHMARK("kl to_string")
    {
        return kl::reflect<abcd>().to_string(abcd(c - 1));
    };

#if defined(KL_BENCH_BOOST_DESCRIBE)
    BENCHMARK("boost.describe enum_to_string")
    {
        return boost::describe::enum_to_string(abcd(c - 1), "(unknown)");
    };
#endif

    BENCHMARK("kl from_string")
    {
        return kl::reflect<abcd>().from_string(v);
    };

#if defined(KL_BENCH_BOOST_DESCRIBE)
    BENCHMARK("boost.describe enum_from_string")
    {
        return my_enum_from_string<abcd>(v);
    };
#endif
}
