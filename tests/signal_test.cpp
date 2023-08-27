#include "kl/signal.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace test {

void foo(bool& z) { z = !z; }

struct Baz
{
    Baz() = default;
    Baz(const Baz&) = delete;

    void bar(int& z) { ++z; }
    void bar(int& z) const { ++z; }
    static void foo(bool& z) { z = !z; }
};
} // namespace test

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

    SECTION("connect null function")
    {
        kl::signal<float(int)> s;
        s.connect(nullptr);
        REQUIRE(s.empty());
        s.connect_extended(nullptr);
        REQUIRE(s.empty());
    }

    SECTION("connect lambda")
    {
        static const int arg = 2;
        kl::signal<float(int)> s;
        s += [&](int a) {
            REQUIRE(arg == a);
            return a * 3.14f;
        };
        REQUIRE(!s.empty());
        REQUIRE(s.num_slots() == 1);

        s(arg);
        s(arg, [&](float srv) { REQUIRE(arg * 3.14f == Catch::Approx(srv)); });

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
        s += l;
        s += l;
        REQUIRE(!s.empty());
        REQUIRE(s.num_slots() == 2);

        s(arg);

        int srvCounter = 0;
        s(arg, [&](float srv) {
            ++srvCounter;
            REQUIRE(3.14f * arg == Catch::Approx(srv));
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
        s += &test::foo;

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
        s += kl::make_slot(&test::Baz::bar, &obj);
        s += kl::make_slot(&test::Baz::bar, const_cast<const test::Baz*>(&obj));

        test::Baz& ref = obj;
        s += kl::make_slot(&test::Baz::bar, ref);

        const test::Baz& cref = obj;
        s += kl::make_slot(&test::Baz::bar, cref);

        // use bind directly
        s += std::bind(static_cast<void (test::Baz::*)(int&)>(&test::Baz::bar),
                       std::ref(obj), std::placeholders::_1);
        s += std::bind(
            static_cast<void (test::Baz::*)(int&) const>(&test::Baz::bar),
            std::cref(obj), std::placeholders::_1);
        int z = 0;
        s(z);
        REQUIRE(z == 6);
    }

    SECTION("make slot shared_ptr to signal")
    {
        auto obj = std::make_shared<test::Baz>();
        kl::signal<void(int&)> s;
        s += kl::make_slot(&test::Baz::bar, obj);
        s += kl::make_slot(&test::Baz::bar, std::weak_ptr<test::Baz>(obj));
        CHECK(obj.use_count() == 2);
        int z = 0;
        s(z);
        CHECK(z == 2);
    }

    SECTION("connect static member function")
    {
        kl::signal<void(bool&)> s;
        s += &test::Baz::foo;

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
        auto c = s += &test::Baz::foo;

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
        s += [&] {
            shot = true;
            return 99;
        };
        s();
        REQUIRE(shot);
    }

    SECTION("sink for void and nonvoid return types")
    {
        kl::signal<void()> s0;
        kl::signal<int()> s1;

        s0 += [] {};
        s1 += [] { return 99; };

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
                                           const std::string&) {
            conn.disconnect();
            ++counter;
        };

        kl::signal<void(const std::string&)> s;
        s.connect_extended(single_shot_slot);

        s("hello");
        s(" world\n"); // Wont be called

        REQUIRE(counter == 1);
    }

    SECTION("connect signal to signal")
    {
        kl::signal<void(int)> s;
        kl::signal<void(int)> t;

        t += [](int i) { REQUIRE(i == 2); };
        s += kl::make_slot(t);
        s(2);
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
        kl::connection c = s += [&] { ++cnt; };
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
        kl::connection c1 = s += [] {};
        kl::connection c2 = s += [] {};

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
        kl::connection c1 = s += [&] { ++cnt; };
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
        kl::connection c1 = s += [&] { ++cnt; };

        {
            auto b = c1.get_blocker();
            auto b2 = c1.get_blocker();
            s();
            REQUIRE(cnt == 0);

            b2 = b;
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
        kl::connection c1 = s += [&] { ++cnt; };

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
            kl::scoped_connection sc = s += [] {};
            REQUIRE(!s.empty());
        }
        REQUIRE(s.empty());
    }

    SECTION("release before the end of scope")
    {
        kl::signal<void()> s;
        kl::connection c;
        {
            kl::scoped_connection sc = s += [] {};
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
    sig += &test::product;
    sig += &test::quotient;
    sig += &test::sum;
    sig += &test::difference;

    SECTION("optional last value")
    {
        // The default combiner returns a std::optional containing the return
        // value of the last slot in the slot list, in this case the
        // difference function.
        std::optional<float> srv;
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
        REQUIRE(srv == Catch::Approx(15.0f));
    }

    SECTION("aggregate values")
    {
        std::vector<float> srv;
        sig(5, 3, [&](float v) { srv.push_back(v); });

        REQUIRE(srv.size() == 4);
        REQUIRE(srv[0] == Catch::Approx(15.0f));
        REQUIRE(srv[1] == Catch::Approx(1.666666667f));
        REQUIRE(srv[2] == Catch::Approx(8.0f));
        REQUIRE(srv[3] == Catch::Approx(2.0f));
    }
}

TEST_CASE("stopping signal emission")
{
    kl::signal<int()> signal;
    signal += [] { return 10; };
    signal += [] { return 100; };
    signal += [] { return 1000; };

    SECTION("run all")
    {
        int cnt = 0;
        signal([&](int) {
            ++cnt;
            return false;
        });
        REQUIRE(cnt == 3);
    }

    SECTION("distribute request")
    {
        int rv = 0;
        signal([&](int ret) {
            rv = ret;
            if (ret > 50)
                return true; // Dont bother calling next slot
            return false;
        });
        REQUIRE(rv == 100); // 3rd slot not called
    }

    SECTION("sink with void returning signal")
    {
        int cnt = 0;
        kl::signal<void()> s;
        s += [&] { cnt++; };
        s += [&] { cnt++; };
        s += [&] { cnt++; };

        s([&] { return cnt >= 1; });

        REQUIRE(cnt == 1);
    }
}

TEST_CASE("connect/disconnect during signal emission")
{
    class Test
    {
    public:
        Test()
        {
            c0 = sig += [&] {
                trace.push_back(0);
                do_run();
                return 0;
            };

            c1 = sig += [&] {
                REQUIRE(false); // never called because c1.disconnect()
                do_run();
                return 0;
            };

            c2 = sig += [&] {
                trace.push_back(2);
                do_run();
                return 0;
            };
        }

        void run() { sig(); }

    private:
        void do_run()
        {
            trace.push_back(99);
            sig += [&] {
                // This is called only during 2nd run
                trace.push_back(4);
                do_run();
                return 1;
            };
            c1.disconnect();
        }

        kl::signal<int()> sig;
        kl::connection c0;
        kl::connection c1;
        kl::connection c2;

    public:
        std::vector<int> trace;
    };

    Test test;
    test.run();
    REQUIRE(test.trace == (std::vector<int>{0, 99, 2, 99}));
    test.run();
    REQUIRE(test.trace ==
            (std::vector<int>{0, 99, 2, 99, 0, 99, 2, 99, 4, 99, 4, 99}));
}

TEST_CASE("one-time slot connection with a next emission in a handler")
{
    kl::signal<void()> s;
    kl::connection c;
    int i = 0;

    SECTION("base case")
    {
        c = s += [&] {
            c.disconnect();
            ++i;
        };
        s += [&] { ++i; };
        s += [&] { ++i; };

        s();
        CHECK(i == 3);
        s();
        CHECK(i == 5);
    }

    SECTION("emit after disconnect")
    {
        bool re_emit = true;
        c = s += [&] {
            ++i;
            if (re_emit)
            {
                re_emit = false;
                s();
            }
        };

        s += [&] { ++i; };
        s += [&] { ++i; };

        s();
        CHECK(i == 6);
    }

    SECTION("emit after disconnect")
    {
        c = s += [&] {
            c.disconnect();
            ++i;
            s();
        };

        s += [&] { ++i; };
        s += [&] { ++i; };

        s();
        CHECK(i == 5);
    }
}

TEST_CASE("add slot to signal during emission")
{
    kl::signal<void()> s;
    kl::connection c;
    int i = 0;

    SECTION("add at back")
    {
        s += [&] {
            ++i;
            s.connect([&] { ++i; });
        };
        s += [&] { ++i; };
        s += [&] { ++i; };

        s();
        CHECK(i == 3);

        s();
        CHECK(i == 7);

        s();
        CHECK(i == 12);
    }

    SECTION("add at front")
    {
        s += [&] {
            ++i;
            s.connect([&] { ++i; }, kl::at_front);
        };
        s += [&] { ++i; };
        s += [&] { ++i; };

        s();
        CHECK(i == 3);

        s();
        CHECK(i == 7);

        s();
        CHECK(i == 12);
    }
}

TEST_CASE("by value vs by const ref")
{
    using kl::signal;

    SECTION("exact match slot and signal signature")
    {
        signal<void(std::vector<int>)> s0;
        signal<void(const std::vector<int>&)> s1;

        // We should get exaclty one copy constructor and one move constructor
        // for each slot invocation (+1 move for extended connection)
        s0 += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };
        s0 += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };
        s0.connect_extended([](kl::connection, std::vector<int> vec) {
            REQUIRE(vec.size() == 5);
        });
        s0.connect_extended([](kl::connection, std::vector<int> vec) {
            REQUIRE(vec.size() == 5);
        });

        // No copy/move ctor in this case
        s1 += [](const std::vector<int>& vec) { REQUIRE(vec.size() == 7); };
        s1 += [](const std::vector<int>& vec) { REQUIRE(vec.size() == 7); };
        s1.connect_extended([](kl::connection, const std::vector<int>& vec) {
            REQUIRE(vec.size() == 7);
        });
        s1.connect_extended([](kl::connection, const std::vector<int>& vec) {
            REQUIRE(vec.size() == 7);
        });

        s0(std::vector<int>{0, 1, 2, 3, 4}, [] {});
        s0(std::vector<int>{0, 1, 2, 3, 4});

        std::vector<int> vec{0, 1, 2, 3, 4, 5, 6};
        s1(vec, [] {});
        s1(vec);
    }

    SECTION("slot by value, signal by const-ref")
    {
        // We should get exaclty one copy constructor for each slot invocation
        signal<void(const std::vector<int>&)> s;

        s += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };
        s += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };

        s(std::vector<int>{0, 1, 2, 3, 4}, [] {});
        s(std::vector<int>{0, 1, 2, 3, 4});
    }
}

TEST_CASE("make_slot and KL_SLOT macro inside class contructor")
{
    using kl::signal;

    signal<void(int)> s;

    class C
    {
    public:
        C(signal<void(int)>& s)
        {
            s += kl::make_slot(&C::handler, this);
            C& inst = *this;
            s += kl::make_slot(&C::handler, inst);
#if defined(KL_SLOT)
            s += KL_SLOT(handler);
#endif
        }

    private:
        void handler(int i)
        {
            REQUIRE(i == 3);
            ++cnt;
        }

    public:
        int cnt = 0;
    };

    C c{s};
    s(3);

#if defined(KL_SLOT)
    REQUIRE(c.cnt == 3);
#else
    REQUIRE(c.cnt == 2);
#endif
}

TEST_CASE("disconnect all slots during emission")
{
    kl::signal<void()> s;
    int i = 0;

    SECTION("disconnect on first slot")
    {
        auto callback = [&] {
            s.disconnect_all_slots();
            ++i;
        };
        s += callback;
        s += callback;

        s();
        CHECK(i == 1);

        s();
        CHECK(i == 1);
    }

    SECTION("disconnect in the middle")
    {
        s += [&] { ++i; };
        s += [&] {
            ++i;
            s.disconnect_all_slots();
        };
        s += [&] { ++i; };

        s();
        CHECK(i == 2);
        CHECK(s.num_slots() == 0);

        s();
        CHECK(i == 2);
    }
}
