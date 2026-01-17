#include "kl/signal.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <thread>
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

TEST_CASE("signal - base functionality", "[signal]")
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
        s.connect(&test::Baz::bar, &obj);
        s.connect(&test::Baz::bar, const_cast<const test::Baz*>(&obj));

        test::Baz& ref = obj;
        s.connect(&test::Baz::bar, ref);

        const test::Baz& cref = obj;
        s.connect(&test::Baz::bar, cref);

        auto shared_ob = std::make_shared<const test::Baz>();
        s.connect(&test::Baz::bar, shared_ob);

        // use bind directly
        s += std::bind(static_cast<void (test::Baz::*)(int&)>(&test::Baz::bar),
                       std::ref(obj), std::placeholders::_1);
        s += std::bind(
            static_cast<void (test::Baz::*)(int&) const>(&test::Baz::bar),
            std::cref(obj), std::placeholders::_1);
        int z = 0;
        s(z);
        REQUIRE(z == 7);
    }

    SECTION("connect shared_ptr instance to a signal")
    {
        auto obj = std::make_shared<test::Baz>();
        kl::signal<void(int&)> s;
        auto c = s.connect(&test::Baz::bar, obj);
        s.connect(&test::Baz::bar, std::weak_ptr<test::Baz>(obj));
        s.connect(&test::Baz::bar, std::weak_ptr{obj});
        s.connect(&test::Baz::bar, std::weak_ptr<const test::Baz>{obj});
        CHECK(obj.use_count() == 2);
        int z = 0;
        s(z);
        CHECK(z == 4);

        obj.reset();
        c.disconnect();
        s(z);
        CHECK(z == 4);
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
        s.connect(t);
        s(2);
        REQUIRE(done);
    }
}

TEST_CASE("signal - connection", "[signal]")
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

TEST_CASE("signal - scoped_connection", "[signal]")
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

TEST_CASE("signal - stopping signal emission", "[signal]")
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

TEST_CASE("signal - disconnect during signal emission", "[signal]")
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

TEST_CASE("signal - signal thread safety (emit vs connect/disconnect)", "[signal]")
{
    SECTION("disconnect from another thread while slot is executing")
    {
        kl::signal<void()> s;
        std::atomic<int> calls{0};

        std::mutex m;
        std::condition_variable cv;
        bool entered = false;
        bool proceed = false;

        kl::connection c = s.connect([&] {
            calls.fetch_add(1, std::memory_order_relaxed);
            std::unique_lock<std::mutex> lock{m};
            entered = true;
            cv.notify_one();
            cv.wait(lock, [&] { return proceed; });
        });

        std::thread emitter{[&] { s(); }};

        {
            std::unique_lock<std::mutex> lock{m};
            cv.wait(lock, [&] { return entered; });
        }

        std::thread disconnector{[&] { c.disconnect(); }};
        disconnector.join();

        {
            std::lock_guard<std::mutex> lock{m};
            proceed = true;
        }
        cv.notify_one();
        emitter.join();

        CHECK(calls.load(std::memory_order_relaxed) == 1);
        CHECK(!c.connected());

        s();
        CHECK(calls.load(std::memory_order_relaxed) == 1);
    }

    SECTION("connect from another thread during emission is not called until next emission")
    {
        kl::signal<void()> s;
        std::atomic<int> slot_a_calls{0};
        std::atomic<int> slot_b_calls{0};

        std::mutex m;
        std::condition_variable cv;
        bool entered = false;
        bool proceed = false;

        s.connect([&] {
            slot_a_calls.fetch_add(1, std::memory_order_relaxed);
            std::unique_lock<std::mutex> lock{m};
            entered = true;
            cv.notify_one();
            cv.wait(lock, [&] { return proceed; });
        });

        std::thread emitter{[&] { s(); }};

        {
            std::unique_lock<std::mutex> lock{m};
            cv.wait(lock, [&] { return entered; });
        }

        // Connect B while A is executing; B must not run in this emission.
        s.connect([&] { slot_b_calls.fetch_add(1, std::memory_order_relaxed); });

        {
            std::lock_guard<std::mutex> lock{m};
            proceed = true;
        }
        cv.notify_one();
        emitter.join();

        CHECK(slot_a_calls.load(std::memory_order_relaxed) == 1);
        CHECK(slot_b_calls.load(std::memory_order_relaxed) == 0);

        s();
        CHECK(slot_a_calls.load(std::memory_order_relaxed) == 2);
        CHECK(slot_b_calls.load(std::memory_order_relaxed) == 1);
    }
}

TEST_CASE("signal - connect and disconnect during signal emission", "[signal]")
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

TEST_CASE("signal - disconnect and recursively emit", "[signal]")
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

TEST_CASE("signal - one-time slot connection with a next emission in a handler", "[signal]")
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

TEST_CASE("signal - add slot to signal during emission", "[signal]")
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

namespace {

struct counter
{
    counter() = default;
    counter(const counter&)
    {
        num_copies++;
    }
    counter& operator=(const counter&)
    {
        num_copies++;
        return *this;
    }
    counter(counter&&)
    {
        num_moves++;
    }
    counter& operator=(counter&&)
    {
        num_moves++;
        return *this;
    }

    static int num_copies;
    static int num_moves;

    static void reset()
    {
        num_copies = 0;
        num_moves = 0;
    }
};
int counter::num_copies{0};
int counter::num_moves{0};

} // namespace

TEST_CASE("signal - by value vs by const ref", "[signal]")
{
    using kl::signal;

    SECTION("exact match slot and signal signature")
    {
        // We should get exaclty one copy constructor and one move constructor
        // for each slot invocation
        signal<void(counter)> s0;
        s0 += [](counter) {};
        s0 += [](counter) {};

        s0(counter{});

        CHECK(counter::num_copies == 2);
        CHECK(counter::num_moves == 2);

        // No copy/move ctor in this case
        signal<void(const counter&)> s1;
        s1 += [](const counter&) {};
        s1 += [](const counter&) {};

        counter::reset();
        counter c;
        s1(c);

        CHECK(counter::num_copies == 0);
        CHECK(counter::num_moves == 0);
    }

    SECTION("slot by value, signal by const-ref")
    {
        // We should get exaclty one copy constructor for each slot invocation
        signal<void(const counter&)> s;

        s += [](counter) {};
        s += [](counter) {};
        s(counter{});

        CHECK(counter::num_copies == 2);
        CHECK(counter::num_moves == 0);
    }
}

TEST_CASE("signal - disconnect all slots during emission", "[signal]")
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

TEST_CASE("signal - move signal during emission", "[signal]")
{
    kl::signal<void()> s, s2;
    int i = 0;

    s += [&] {
        ++i;
        using std::swap;
        swap(s, s2);
        s.disconnect_all_slots();
    };
    s += [&] { ++i; };

    s();
    CHECK(i == 2);
    CHECK(s.num_slots() == 0);
    CHECK(s2.num_slots() == 2);

    s2();
    CHECK(i == 3);
    CHECK(s.num_slots() == 0);
    CHECK(s2.num_slots() == 0);
}

namespace {

std::atomic<std::int64_t> sum{0};

void f(int i)
{
    sum += i;
}
void f1(int i)
{
    sum += i;
}
void f2(int i)
{
    sum += i;
}
void f3(int i)
{
    sum += i;
}

void emit_many(kl::signal<void(int)>& sig)
{
    for (int i = 0; i < 10000; ++i)
        sig(1);
}

void connect_emit(kl::signal<void(int)>& sig)
{
    for (int i = 0; i < 100; ++i)
    {
        kl::scoped_connection c = sig.connect(f);
        for (int j = 0; j < 100; ++j)
            sig(1);
    }
}

void connect_cross(kl::signal<void(int)>& s1, kl::signal<void(int)>& s2,
                   std::atomic<int>& go)
{
    auto cross = s1.connect([&](int i) {
        if (i & 1)
            f(i);
        else
            s2(i + 1);
    });

    go++;
    while (go != 3)
        std::this_thread::yield();

    for (int i = 0; i < 1000000; ++i)
        s1(i);
}

} // namespace

TEST_CASE("signal - multithreading", "[signal]")
{
    SECTION("threaded mix")
    {
        sum = 0;

        kl::signal<void(int)> sig;

        std::array<std::thread, 10> threads;
        for (auto& t : threads)
            t = std::thread(connect_emit, std::ref(sig));

        for (auto& t : threads)
            t.join();
    }

    SECTION("threaded emission")
    {
        sum = 0;

        kl::signal<void(int)> sig;
        sig.connect(f);

        std::array<std::thread, 10> threads;
        for (auto& t : threads)
            t = std::thread(emit_many, std::ref(sig));

        for (auto& t : threads)
            t.join();

        CHECK(sum == 100000l);
    }

    SECTION("cross connections")
    {
        sum = 0;

        kl::signal<void(int)> sig1;
        kl::signal<void(int)> sig2;

        std::atomic<int> go{0};

        std::thread t1(connect_cross, std::ref(sig1), std::ref(sig2),
                       std::ref(go));
        std::thread t2(connect_cross, std::ref(sig2), std::ref(sig1),
                       std::ref(go));

        while (go != 2)
            std::this_thread::yield();
        go++;

        t1.join();
        t2.join();

        CHECK(sum == 1000000000000ll);
    }

    SECTION("threaded misc")
    {
        sum = 0;
        kl::signal<void(int)> sig;
        std::atomic<bool> run{true};

        std::mutex connections_mutex;
        std::vector<kl::connection> connections;
        connections.reserve(4096);

        auto emitter = [&] {
            while (run)
            {
                sig(1);
            }
        };

        auto conn = [&] {
            while (run)
            {
                for (int i = 0; i < 10; ++i)
                {
                    auto c1 = sig.connect(f1);
                    auto c2 = sig.connect(f2);
                    auto c3 = sig.connect(f3);

                    std::scoped_lock lock(connections_mutex);
                    connections.push_back(std::move(c1));
                    connections.push_back(std::move(c2));
                    connections.push_back(std::move(c3));
                    if (connections.size() > 8192)
                    {
                        connections.erase(connections.begin(),
                                          connections.begin() +
                                              (connections.size() - 4096));
                    }
                }
            }
        };

        auto disconn = [&] {
            while (run)
            {
                kl::connection c;
                {
                    std::scoped_lock lock(connections_mutex);
                    if (connections.empty())
                        continue;
                    c = std::move(connections.back());
                    connections.pop_back();
                }
                c.disconnect();
            }
        };

        std::array<std::thread, 20> emitters;
        std::array<std::thread, 20> conns;
        std::array<std::thread, 20> disconns;

        for (auto& t : conns)
            t = std::thread(conn);
        for (auto& t : emitters)
            t = std::thread(emitter);
        for (auto& t : disconns)
            t = std::thread(disconn);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        run = false;

        for (auto& t : emitters)
            t.join();
        for (auto& t : disconns)
            t.join();
        for (auto& t : conns)
            t.join();
    }
}
