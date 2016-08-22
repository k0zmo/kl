#pragma once

#include <memory>
#include <tuple>
#include <functional>
#include <forward_list>
#include <cassert>
#include <type_traits>
#include <algorithm>

namespace kl {

class connection;

namespace detail {

class signal_base
{
protected:
    friend class kl::connection;

    signal_base() = default;
    ~signal_base() = default;

    signal_base(signal_base&&) {}
    signal_base& operator=(signal_base&&) { return *this; }

private:
    signal_base(const signal_base&) = delete;
    signal_base& operator=(const signal_base&) = delete;

    virtual void disconnect(int id) = 0;
};

struct connection_info final
{
    connection_info(signal_base* parent, int id) : parent{parent}, id{id} {}

    signal_base* parent;
    const int id;
    unsigned blocking{0};
};

template <typename T>
size_t hash_std(T&& v)
{
    return std::hash<typename std::decay<T>::type>{}(std::forward<T>(v));
}

template <typename T>
struct identity_type
{
    using type = T;
};
} // namespace detail

/*
 * Represent a connection from given signal to a slot. Can be used to disconnect
 * the connection at any time later.
 * connection object can be kept inside standard containers (e.g unordered_set)
 * compared between each others and finally move/copy around.
 */
class connection final
{
public:
    // Creates empty connection
    connection() = default;
    ~connection() = default;

    // Copy constructor/operator
    connection(const connection&) = default;
    connection& operator=(const connection&) = default;

    // Move constructor/operator
    connection(connection&& other) : impl_{std::move(other.impl_)} {}
    connection& operator=(connection&& other)
    {
        swap(*this, other);
        return *this;
    }

    // Disconnects the connection. No-op if connection is already disconnected.
    void disconnect()
    {
        if (!connected())
            return;
        impl_->parent->disconnect(impl_->id);
        impl_ = nullptr;
    }

    // Returns true if connection is valid
    bool connected() const { return impl_ && impl_->parent; }

    size_t hash() const
    {
        // Get hash out of underlying pointer
        return detail::hash_std(impl_);
    }

    class blocker
    {
    public:
        blocker() = default;
        blocker(connection& con) : impl_{con.impl_}
        {
            if (impl_)
                ++impl_->blocking;
        }

        ~blocker()
        {
            if (impl_)
                --impl_->blocking;
        }

        blocker(const blocker& other) : impl_{other.impl_}
        {
            if (impl_)
                ++impl_->blocking;
        }

        blocker& operator=(const blocker& other)
        {
            if (this != &other)
            {
                if (impl_)
                    --impl_->blocking;
                impl_ = other.impl_;
                if (impl_)
                    ++impl_->blocking;
            }
            return *this;
        }

        blocker(blocker&& other) : impl_{std::move(other.impl_)} {}

        blocker& operator=(blocker&& other)
        {
            swap(*this, other);
            return *this;
        }

        bool blocking() const { return impl_ != nullptr; }

        friend void swap(blocker& left, blocker& right)
        {
            using std::swap;
            swap(left.impl_, right.impl_);
        }

    private:
        std::shared_ptr<detail::connection_info> impl_;
    };

    blocker get_blocker() { return {*this}; }

public:
    friend void swap(connection& left, connection& right)
    {
        using std::swap;
        swap(left.impl_, right.impl_);
    }

    friend bool operator==(const connection& left, const connection& right)
    {
        return left.impl_ == right.impl_;
    }

    friend bool operator<(const connection& left, const connection& right)
    {
        return left.impl_ < right.impl_;
    }

private:
    friend connection
        make_connection(std::shared_ptr<detail::connection_info> ci);
    // Private constructor - use by concrete instances of signal class
    connection(std::shared_ptr<detail::connection_info> c) : impl_{std::move(c)}
    {
        assert(impl_);
    }

private:
    std::shared_ptr<detail::connection_info> impl_;
};

inline connection make_connection(std::shared_ptr<detail::connection_info> ci)
{
    return connection(std::move(ci));
}

/*
 * RAII connection. Disconnects underlying connection upon destruction.
 * Similarly, can be kept inside standard containers, compare and move around
 */
class scoped_connection final
{
public:
    scoped_connection() = default;
    scoped_connection(connection connection)
        : connection_{std::move(connection)}
    {
    }

    ~scoped_connection() { connection_.disconnect(); }

    scoped_connection(const scoped_connection&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;

    scoped_connection(scoped_connection&& other)
        : connection_{std::move(other.connection_)}
    {
    }

    scoped_connection& operator=(scoped_connection&& other)
    {
        swap(*this, other);
        return *this;
    }

    // Release underlying connection. Won't disconnect it upon destruction
    connection release()
    {
        connection ret{std::move(connection_)};
        return ret;
    }

    const connection& get() const { return connection_; }

    friend void swap(scoped_connection& left, scoped_connection& right)
    {
        using std::swap;
        swap(left.connection_, right.connection_);
    }

    friend bool operator==(const scoped_connection& left,
                           const scoped_connection& right)
    {
        return left.connection_ == right.connection_;
    }

    friend bool operator<(const scoped_connection& left,
                          const scoped_connection& right)
    {
        return left.connection_ < right.connection_;
    }

private:
    connection connection_;
};

enum connect_position
{
    at_back,
    at_front
};

namespace detail {

template <typename Ret>
struct sink_invoker
{
private:
    template <typename Sink, typename Slot, typename... Args>
    static bool call_impl(std::false_type /*is_sink_return_type_void*/,
                          Sink&& sink, const Slot& slot, const Args&... args)
    {
        return !!std::forward<Sink>(sink)(slot(args...));
    }

    template <typename Sink, typename Slot, typename... Args>
    static bool call_impl(std::true_type /*is_sink_return_type_void*/,
                          Sink&& sink, const Slot& slot, const Args&... args)
    {
        std::forward<Sink>(sink)(slot(args...));
        return false;
    }

public:
    template <typename Sink, typename Slot, typename... Args>
    static bool call(Sink&& sink, const Slot& slot, const Args&... args)
    {
        using is_sink_return_type_void = std::integral_constant<
            bool, std::is_same<void, std::result_of_t<Sink(
                                         typename Slot::return_type)>>::value>;

        return sink_invoker::call_impl(is_sink_return_type_void{},
                                       std::forward<Sink>(sink), slot, args...);
    }
};

template <>
struct sink_invoker<void>
{
private:
    template <typename Sink, typename Slot, typename... Args>
    static bool call_impl(std::false_type /*is_sink_return_type_void*/,
                          Sink&& sink, const Slot& slot, const Args&... args)
    {
        slot(args...);
        return !!std::forward<Sink>(sink)();
    }

    template <typename Sink, typename Slot, typename... Args>
    static bool call_impl(std::true_type /*is_sink_return_type_void*/,
                          Sink&& sink, const Slot& slot, const Args&... args)
    {
        slot(args...);
        std::forward<Sink>(sink)();
        return false;
    }

public:
    template <typename Sink, typename Slot, typename... Args>
    static bool call(Sink&& sink, const Slot& slot, const Args&... args)
    {
        using is_sink_return_type_void = std::integral_constant<
            bool, std::is_same<void, std::result_of_t<Sink()>>::value>;

        return sink_invoker::call_impl(is_sink_return_type_void{},
                                       std::forward<Sink>(sink), slot, args...);
    }
};
} // namespace detail

template <typename Signature>
class signal;

template <typename Ret, typename... Args>
class signal<Ret(Args...)> : public detail::signal_base
{
public:
    // MSVC2013 doesn't like: using signature_type = Ret(Args...);
    using signature_type = typename detail::identity_type<Ret(Args...)>::type;
    using slot_type = std::function<Ret(Args...)>;
    using extended_slot_type = std::function<Ret(connection&, Args...)>;
    using signal_type = signal<Ret(Args...)>;
    using return_type = Ret;
    static const size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity, "invalid arg index");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

public:
    // Constructs empty, disconnected signal
    signal() = default;
    ~signal() { disconnect_all_slots(); }

    signal(signal&& other) : slots_(std::move(other.slots_)), id_{other.id_}
    {
        rebind();
    }

    signal& operator=(signal&& other)
    {
        swap(*this, other);
        return *this;
    }

    connection connect_extended(extended_slot_type extended_slot,
                                connect_position at = at_back)
    {
        if (!extended_slot)
            return {};

        auto connection_info =
            std::make_shared<detail::connection_info>(this, ++id_);

        // Create proxy slot that would add connection as a first argument
        auto connection = make_connection(connection_info);
#if (defined(_MSC_VER) && _MSC_VER >= 1900) || __cplusplus >= 201402
        auto proxy_slot = [connection, slot = std::move(extended_slot)]
            (Args&&... args) mutable {
                return slot(connection, std::forward<Args>(args)...);
            };
#else
        auto proxy_slot = [=](Args&&... args) mutable {
            return extended_slot(connection, std::forward<Args>(args)...);
        };
#endif
        slots_.emplace_after(find_slot_place(at), connection_info,
                             std::move(proxy_slot));
        return connection;
    }

    // Connects signal object to given slot object.
    connection connect(slot_type slot, connect_position at = at_back)
    {
        if (!slot)
            return {};
        auto connection_info =
            std::make_shared<detail::connection_info>(this, ++id_);
        slots_.emplace_after(find_slot_place(at), connection_info,
                             std::move(slot));
        return make_connection(std::move(connection_info));
    }

    // Emits signal
    void operator()(Args... args)
    {
        call_each_slot([&](const slot& s) {
            s(args...);
            return false;
        });
    }

    // Emits signal and sinks signal return values
    template <typename Sink>
    void operator()(Args... args, Sink&& sink)
    {
        call_each_slot([&](const slot& s) {
            using invoker = detail::sink_invoker<return_type>;
            return invoker::call(std::forward<Sink>(sink), s, args...);
        });
    }

    // Disconnects all slots bound to this signal
    void disconnect_all_slots()
    {
        for (auto& slot : slots_)
            slot.invalidate();
        slots_.clear();
    }

    // Retrieves number of slots connected to this signal
    size_t num_slots() const
    {
        return std::count_if(slots_.begin(), slots_.end(),
                             [](const slot& s) { return s.valid(); });
    }

    // Returns true if signal isn't connected to any slot
    bool empty() const { return num_slots() == 0; }

    friend void swap(signal& left, signal& right)
    {
        using std::swap;
        swap(left.id_, right.id_);
        swap(left.slots_, right.slots_);

        left.rebind();
        right.rebind();
    }

private:
    virtual void disconnect(int id) override
    {
        auto target =
            std::find_if(slots_.begin(), slots_.end(), [&](const slot& slot) {
                return slot.connection_info().id == id;
            });
        if (target != slots_.end())
            target->invalidate();

        // We can't delete the node here since we could be in the middle of
        // signal emission or we are called from extended slot. In that case it
        // would lead to removal of connection (proxy slot keeps the copy) that
        // is invoking this very function.
    }

    void rebind()
    {
        slots_.remove_if([&](const slot& slot) {
            return !slot.valid();
        });

        // Rebind back-pointer to new signal
        for (auto& slot : slots_)
        {
            assert(slot.connection_info().parent);
            slot.connection_info().parent = this;
        }
    }

    void prepare_slots()
    {
        auto it_before = slots_.before_begin(),
             it = slots_.begin(),
             last = slots_.end();

        while (it != last)
        {
            if (!it->prepare())
            {
                it = slots_.erase_after(it_before);
                continue;
            }

            ++it_before;
            ++it;
        }
    }

    template <typename Func>
    void call_each_slot(Func&& func)
    {
        prepare_slots();

        for (const auto& slot : slots_)
        {
            if (!slot.is_blocked() && slot.is_prepared())
            {
                if (func(slot))
                    break;
            }
        }
    }

private:
    class slot final
    {
    public:
        using return_type = typename signal::return_type;

    public:
        slot(std::shared_ptr<detail::connection_info> connection_info,
             slot_type impl)
            : connection_info_{std::move(connection_info)},
              impl_{std::move(impl)}

        {
        }

        slot(const slot&) = delete;
        slot& operator=(const slot&) = delete;

        return_type operator()(const Args&... args) const
        {
            return impl_(args...);
        }

        bool prepare()
        {
            if (!valid())
                return false;
            prepared_ = true;
            return true;
        }

        void invalidate() { connection_info().parent = nullptr; }

        const detail::connection_info& connection_info() const
        {
            return *connection_info_;
        }

        detail::connection_info& connection_info() { return *connection_info_; }

        bool valid() const { return connection_info().parent != nullptr; }
        bool is_blocked() const { return connection_info().blocking > 0; }
        bool is_prepared() const { return valid() && prepared_; }

    private:
        std::shared_ptr<detail::connection_info> connection_info_;
        slot_type impl_;
        bool prepared_{false};
    };

    std::forward_list<slot> slots_;
    int id_{0};

private:
    typename std::forward_list<slot>::iterator
        find_slot_place(connect_position at)
    {
        // We want to insert/emplace at the end of the list
        auto iter = slots_.before_begin();
        if (at == at_back)
        {
            for (auto& _ : slots_)
            {
                (void)_; // Suppress "unreferenced variable"
                ++iter;
            }
        }

        return iter;
    }
};

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...), T* instance)
{
    return {[=](Args&&... args) {
        return (instance->*mem_func)(std::forward<Args>(args)...);
    }};
}

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...) const,
                                      const T* instance)
{
    return {[=](Args&&... args) {
        return (instance->*mem_func)(std::forward<Args>(args)...);
    }};
}

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...), T& instance)
{
    return {[mem_func, &instance](Args&&... args) {
        return (const_cast<T&>(instance).*mem_func)(std::forward<Args>(args)...);
    }};
}

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...) const,
                                      const T& instance)
{
    return {[mem_func, &instance](Args&&... args) {
        return (instance.*mem_func)(std::forward<Args>(args)...);
    }};
}

// Converts signal to slot type allowing to create signal-to-signal connections
template <typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(signal<Ret(Args...)>& sig)
{
    return {[&](Args&&... args) { return sig(std::forward<Args>(args)...); }};
}
} // namespace kl

namespace std {

// Hash support for kl::connection and kl::scoped_connection
template <>
struct hash<kl::connection>
{
    size_t operator()(const kl::connection& key) const { return key.hash(); }
};

template <>
struct hash<kl::scoped_connection>
{
    size_t operator()(const kl::scoped_connection& key) const
    {
        return hash<kl::connection>{}(key.get());
    }
};
} // namespace std

// Handy macro for slot connecting inside a class (requires C++14 generic lambdas)
#if (defined(_MSC_VER) && _MSC_VER >= 1900) || __cplusplus >= 201402
#define KL_SLOT(mem_fn)                                                        \
    [this](auto&&... args) {                                                   \
        return this->mem_fn(std::forward<decltype(args)>(args)...);            \
    }
#endif
