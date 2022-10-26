#include "kl/signal.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <functional>
#include <memory>
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
        kl::signal<void(int)> s;
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
        s.disconnect_all_slots();
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
    }

    SECTION("connect null function")
    {
        kl::signal<void(int)> s;
        s.connect(nullptr);
        REQUIRE(s.empty());
    }

    SECTION("connect lambda")
    {
        static const int arg = 2;
        kl::signal<void(int)> s;
        s += [&](int a) {
            REQUIRE(arg == a);;
        };
        REQUIRE(!s.empty());
        REQUIRE(s.num_slots() == 1);

        s(arg);

        s.disconnect_all_slots();
        REQUIRE(s.empty());
        REQUIRE(s.num_slots() == 0);
    }

    SECTION("connect lambda twice")
    {
        static const int arg = 3;
        kl::signal<void(int)> s;

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
        REQUIRE(counter == 2);

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

    SECTION("current connection")
    {
        int counter = 0;
        auto single_shot_slot = [&counter](const std::string&) {
            auto conn = kl::this_signal::current_connection();
            REQUIRE(conn.connected());
            conn.disconnect();
            ++counter;
        };

        kl::signal<void(const std::string&)> s;
        s.connect(single_shot_slot);

        s("hello");
        s(" world\n"); // Wont be called

        REQUIRE(counter == 1);
    }

    SECTION("current connection, recursive")
    {
        int counter = 0;
        kl::signal<void()> s1;
        kl::signal<void()> s2;
        s1 += [&]() {
            auto conn = kl::this_signal::current_connection();
            REQUIRE(conn.connected());
            ++counter;
            s2();
            CHECK(kl::this_signal::current_connection() == conn);
        };
        s2 += [&]() {
            auto conn = kl::this_signal::current_connection();
            REQUIRE(conn.connected());
            conn.disconnect();
            ++counter;
        };

        s1();
        CHECK(counter == 2);
        s1();
        CHECK(counter == 3);
    }

    SECTION("connect signal to signal")
    {
        kl::signal<void(int)> s;
        kl::signal<void(int)> t;

        bool done = false;
        t += [&](int i) {
            REQUIRE(i == 2);
            done = true;
        };
        s += kl::make_slot(t);
        s(2);
        REQUIRE(done);
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

TEST_CASE("stopping signal emission")
{
    kl::signal<void()> s;
    int i = 0;

    SECTION("a")
    {
        s += [&] {
            i += 10;
            kl::this_signal::stop_emission();
        };
        s += [&] { i += 100; };
        s += [&] { i += 1000; };
        s();
        CHECK(i == 10);
        s();
        CHECK(i == 20);
    }

    SECTION("b")
    {
        s += [&] { i += 10; };
        s += [&] {
            i += 100;
            kl::this_signal::stop_emission();
        };
        s += [&] { i += 1000; };
        s();
        CHECK(i == 110);
        s();
        CHECK(i == 220);
    }

    SECTION("b")
    {
        s += [&] { i += 10; };
        s += [&] { i += 100; };
        s += [&] {
            i += 1000;
            kl::this_signal::stop_emission();
        };
        s();
        CHECK(i == 1110);
        s();
        CHECK(i == 2220);
    }
}

TEST_CASE("disconnect during signal emission")
{
    kl::signal<void()> s;
    int i = 0;

    SECTION("disconnect current and middle")
    {
        kl::connection c;
        s += [&] { ++i; };
        c = s += [&] {
            i += 5;
            c.disconnect();
        };
        s += [&] { ++i; };

        s();
        CHECK(i == 7);
        CHECK(s.num_slots() == 2);

        s();
        CHECK(i == 9);
    }

    SECTION("disconnect current and first")
    {
        kl::connection c;
        c = s += [&] {
            i += 5;
            c.disconnect();
        };
        s += [&] {
            ++i;
        };
        s += [&] { ++i; };

        s();
        CHECK(i == 7);
        CHECK(s.num_slots() == 2);

        s();
        CHECK(i == 9);
    }

    SECTION("disconnect last")
    {
        kl::connection c;
        s += [&] { ++i; };
        s += [&] {
            ++i;
            c.disconnect();
        };
        c = s += [&] { i += 10; };

        s();
        CHECK(i == 2);
        CHECK(s.num_slots() == 2);

        s();
        CHECK(i == 4);
    }
}

TEST_CASE("connect and disconnect during signal emission")
{
    class Test
    {
    public:
        Test()
        {
            c0 = sig += [&] {
                trace.push_back(0);
                do_run();
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

        kl::signal<void()> sig;
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

TEST_CASE("disconnect and recursively emit")
{
    kl::signal<void()> s;
    kl::connection c1, c2;
    int i = 0;
    c1 = s += [&]() {
        i += 3;
        c1.disconnect();
        s();
    };
    c2 = s += [&]() {
        i += 5;
        c2.disconnect();
    };
    s();
    CHECK(i == 8);
    s();
    CHECK(i == 8);
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
        // for each slot invocation
        s0 += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };
        s0 += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };

        // No copy/move ctor in this case
        s1 += [](const std::vector<int>& vec) { REQUIRE(vec.size() == 7); };
        s1 += [](const std::vector<int>& vec) { REQUIRE(vec.size() == 7); };

        s0(std::vector<int>{0, 1, 2, 3, 4});

        std::vector<int> vec{0, 1, 2, 3, 4, 5, 6};
        s1(vec);
    }

    SECTION("slot by value, signal by const-ref")
    {
        // We should get exaclty one copy constructor for each slot invocation
        signal<void(const std::vector<int>&)> s;

        s += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };
        s += [](std::vector<int> vec) { REQUIRE(vec.size() == 5); };

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
