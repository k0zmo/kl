#include "kl/signal.hpp"

#include <catch/catch.hpp>
#include <boost/optional.hpp>
#include <vector>

namespace test {

void foo(bool& z) { z = !z; }

struct Baz
{
    void bar(int& z) { ++z; }
    void bar(int& z) const { ++z; }
    static void foo(bool& z) { z = !z; }
};
}

TEST_CASE("signal")
{
    SECTION("empty signal")
    {
        kl::signal<float(int)> s;
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
        s.disconnect_all_slots();
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
    }

    SECTION("connect lambda")
    {
        static const int arg = 2;
        kl::signal<float(int)> s;
        s.connect([&](int a) {
            REQUIRE(arg == a);
            return a * 3.14f;
        });
        REQUIRE(!s.empty());
        REQUIRE(s.num_slots() == 1);

        s(arg);
        s(arg, [&](float srv) {
            REQUIRE(arg * 3.14f == Approx(srv));
        });

        s.disconnect_all_slots();
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
    }

    SECTION("connect lambda twice")
    {
        static const int arg = 3;
        kl::signal<float(int)> s;

        int counter = 0;

        auto l = [&](int a) {
            ++counter;
            return a * 3.14f;
        };
        s.connect(l);
        s.connect(l);
        REQUIRE(!s.empty());
        REQUIRE(s.num_slots() == 2);

        s(arg);

        int srvCounter = 0;
        s(arg, [&](float srv) {
            ++srvCounter;
            REQUIRE(3.14f * arg == Approx(srv));
        });

        REQUIRE(counter == 2 * 2);
        REQUIRE(srvCounter == 2);

        s.disconnect_all_slots();
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
    }

    SECTION("connect free function")
    {
        kl::signal<void(bool&)> s;
        s.connect(&test::foo);

        bool v = true;
        s(v);
        REQUIRE(!v);
        s(v);
        REQUIRE(v);
    }

    SECTION("connect member functions (const and 'normal')")
    {
        test::Baz obj;
        kl::signal<void(int&)> s;
        s.connect(kl::make_slot(&test::Baz::bar, &obj));
        s.connect(
            kl::make_slot(&test::Baz::bar, const_cast<const test::Baz*>(&obj)));

        test::Baz& ref = obj;
        s.connect(kl::make_slot(&test::Baz::bar, ref));

        const test::Baz& cref = obj;
        s.connect(kl::make_slot(&test::Baz::bar, cref));

        // use bind directly
        s.connect(
            std::bind(static_cast<void (test::Baz::*)(int&)>(&test::Baz::bar),
                std::ref(obj), std::placeholders::_1));
        s.connect(std::bind(
            static_cast<void (test::Baz::*)(int&) const>(&test::Baz::bar),
            std::cref(obj), std::placeholders::_1));
        int z = 0;
        s(z);
        REQUIRE(z == 6);
    }

    SECTION("connect static member function")
    {
        kl::signal<void(bool&)> s;
        s.connect(&test::Baz::foo);

        bool v = false;
        s(v);
        REQUIRE(v);

        s.disconnect_all_slots();
        s(v);
        REQUIRE(v);
    }

    SECTION("move signal object")
    {
        kl::signal<void(bool&)> s;
        auto c = s.connect(&test::Baz::foo);

        bool v{false};
        s(v);
        REQUIRE(v);

        kl::signal<void(bool&)> s2 = std::move(s);
        REQUIRE(s2.num_slots() == 1);
        REQUIRE(s.num_slots() == 0);
        s2(v);
        REQUIRE(!v);
        REQUIRE(c.connected());
        c.disconnect();
        REQUIRE(s2.num_slots() == 0);
    }

    SECTION("connect slot at front")
    {
        int v = 3;
        kl::signal<void(int&)> s;
        s.connect([&](int& v) { v += 3; });
        s.connect([&](int& v) { v *= 3; });

        s(v);
        REQUIRE(v == 18);
        v = 3;

        s.disconnect_all_slots();
        s.connect([&](int& v) { v += 3; }, kl::at_front);
        s.connect([&](int& v) { v *= 3; }, kl::at_front);
        s(v);
        REQUIRE(v == 12);
    }

    SECTION("ignores return type")
    {
        kl::signal<void()> s;
        bool shot{};
        s.connect([&] { shot = true; return 99; });
        s();
        REQUIRE(shot);
    }

    SECTION("sink for void and nonvoid return types")
    {
        kl::signal<void()> s0;
        kl::signal<int()> s1;

        s0.connect([] {});
        s1.connect([] { return 99; });

        s0();
        s1();

        int cnt{};
        s0([&] { ++cnt; });
        s1([](int v) { REQUIRE(v == 99); });
        REQUIRE(cnt == 1);
    }

    SECTION("extended connect")
    {
        int counter = 0;
        auto single_shot_slot = [&counter](kl::connection& conn,
                                           const std::string& message) {
            conn.disconnect();
            ++counter;
        };

        kl::signal<void(const std::string&)> s;
        s.connect_extended(single_shot_slot);

        s("hello");
        s(" world\n"); // Wont be called

        REQUIRE(counter == 1);
    }
}

TEST_CASE("connection")
{
    SECTION("empty connection")
    {
        kl::connection c;
        REQUIRE(!c.connected());
        REQUIRE(!c.get_blocker().blocking());
        c.disconnect();
        REQUIRE(!c.connected());
    }

    SECTION("disconnect a connection")
    {
        kl::signal<void()> s;
        int cnt = 0;
        kl::connection c = s.connect([&] { ++cnt; });
        REQUIRE(c.connected());
        s();
        REQUIRE(cnt == 1);

        c.disconnect();
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
        REQUIRE(!c.connected());
        s();
        REQUIRE(cnt == 1);
    }

    SECTION("hash a connection")
    {
        kl::signal<void()> s;
        kl::connection c1 = s.connect([&] {; });
        kl::connection c2 = s.connect([&] {; });

        auto hasher = std::hash<kl::connection>{};

        REQUIRE(hasher(c1) != hasher(c2));
        REQUIRE(hasher(c1) == hasher(c1));

        auto c3 = c1;
        REQUIRE(hasher(c1) == hasher(c3));
        REQUIRE(c3.connected());
        auto c4 = std::move(c1);
        REQUIRE(hasher(c3) == hasher(c4));
        REQUIRE(c4.connected());

        c4.disconnect();
        REQUIRE(!c4.connected());
        REQUIRE(!c3.connected());
    }

    SECTION("get blocker")
    {
        kl::signal<void()> s;
        int cnt = 0;
        kl::connection c1 = s.connect([&] { ++cnt; });
        s();
        REQUIRE(cnt == 1);

        {
            auto b = c1.get_blocker();
            REQUIRE(b.blocking());
            s();
            REQUIRE(cnt == 1);
        }

        s();
        REQUIRE(cnt == 2);

        c1.disconnect();
        REQUIRE(!c1.get_blocker().blocking());
    }

    SECTION("get blocker twice in the same scope")
    {
        kl::signal<void()> s;
        int cnt = 0;
        kl::connection c1 = s.connect([&] { ++cnt; });

        {
            auto b = c1.get_blocker();
            auto b2 = c1.get_blocker();
            s();
            REQUIRE(cnt == 0);
        }

        s();
        REQUIRE(cnt == 1);
    }

    SECTION("check copy/move operations for blocker")
    {
        kl::signal<void()> s;
        int cnt = 0;
        kl::connection c1 = s.connect([&] { ++cnt; });

        {
            auto b = c1.get_blocker();
            {
                auto b2 = b;
                s();
                REQUIRE(cnt == 0);

                kl::connection::blocker b3;
                b3 = b2;
                s();
                REQUIRE(cnt == 0);
            }

            s();
            REQUIRE(cnt == 0);

            {
                auto b2 = std::move(b);
                s();
                REQUIRE(cnt == 0);

                kl::connection::blocker b3;
                b3 = std::move(b2);
                s();
                REQUIRE(cnt == 0);
            }

            s();
            REQUIRE(cnt == 1);
        }

        s();
        REQUIRE(cnt == 2);
    }
}

TEST_CASE("scoped_connection")
{
    SECTION("empty")
    {
        kl::scoped_connection sc;
        REQUIRE(!sc.get().connected());
    }

    SECTION("end of the scope")
    {
        kl::signal<void()> s;
        {
            kl::scoped_connection sc = s.connect([&] {});
            REQUIRE(!s.empty());
        }
        REQUIRE(s.empty());
    }

    SECTION("release before the end of scope")
    {
        kl::signal<void()> s;
        kl::connection c;
        {
            kl::scoped_connection sc = s.connect([&] {});
            REQUIRE(!s.empty());
            c = sc.release();
        }
        REQUIRE(!s.empty());
        c.disconnect();
        REQUIRE(s.empty());
    }
}

namespace test {

bool foo() { return false; }
bool bar() { return true; }

float product(float x, float y) { return x * y; }
float quotient(float x, float y) { return x / y; }
float sum(float x, float y) { return x + y; }
float difference(float x, float y) { return x - y; }
} // namespace test

TEST_CASE("signal combiners")
{
    // http://www.boost.org/doc/libs/1_60_0/doc/html/signals2/rationale.html
    // We use push model - with lambdas and all it's not that bad

    kl::signal<float(float, float)> sig;
    sig.connect(&test::product);
    sig.connect(&test::quotient);
    sig.connect(&test::sum);
    sig.connect(&test::difference);

    SECTION("optional last value")
    {
        // The default combiner returns a boost::optional containing the return
        // value of the last slot in the slot list, in this case the
        // difference function.
        boost::optional<float> srv;
        sig(5, 3, [&](float v) { srv = v; });
        REQUIRE(srv);
        REQUIRE(*srv == 2);
    }

    SECTION("maximum")
    {
        float srv{};
        sig(5, 3, [&](float v) { srv = std::max(srv, v); });

        // Outputs the maximum value returned by the connected slots, in this
        // case 15 from the product function.
        REQUIRE(srv == Approx(15.0f));
    }

    SECTION("aggregate values")
    {
        std::vector<float> srv;
        sig(5, 3, [&](float v) { srv.push_back(v); });

        REQUIRE(srv.size() == 4);
        REQUIRE(srv[0] == Approx(15.0f));
        REQUIRE(srv[1] == Approx(1.666666667f));
        REQUIRE(srv[2] == Approx(8.0f));
        REQUIRE(srv[3] == Approx(2.0f));
    }
}
