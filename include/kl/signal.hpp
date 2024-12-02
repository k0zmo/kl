#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace kl {

class connection;

namespace detail {

struct signal_base
{
    virtual ~signal_base() = default;
    virtual void disconnect(std::uintptr_t id) = 0;
};

struct connection_info final
{
    connection_info(signal_base* sender) noexcept
        : sender{sender}
    {
    }

    signal_base* sender;
    std::uintptr_t id{0};
    unsigned blocking{0};
};

template <typename T>
size_t hash_std(T&& v)
{
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::hash<remove_cvref_t>{}(std::forward<T>(v));
}
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
    connection() noexcept = default;
    ~connection() = default;

    // Copy constructor/operator
    connection(const connection&) = default;
    connection& operator=(const connection&) = default;

    // Move constructor/operator
    connection(connection&& other) noexcept
        : impl_{std::exchange(other.impl_, nullptr)}
    {
    }
    connection& operator=(connection&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    // Disconnects the connection. No-op if connection is already disconnected.
    void disconnect()
    {
        if (!connected())
            return;
        impl_->sender->disconnect(impl_->id);
        impl_ = nullptr;
    }

    // Returns true if connection is valid
    bool connected() const { return impl_ && impl_->sender; }

    size_t hash() const noexcept
    {
        // Get hash out of underlying pointer
        return detail::hash_std(impl_);
    }

    class blocker
    {
    public:
        blocker() noexcept = default;
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

        blocker(blocker&& other) noexcept : impl_{std::move(other.impl_)} {}

        blocker& operator=(blocker&& other) noexcept
        {
            swap(*this, other);
            return *this;
        }

        bool blocking() const noexcept { return impl_ != nullptr; }

        friend void swap(blocker& left, blocker& right) noexcept
        {
            using std::swap;
            swap(left.impl_, right.impl_);
        }

    private:
        std::shared_ptr<detail::connection_info> impl_;
    };

    blocker get_blocker() { return {*this}; }

public:
    friend void swap(connection& left, connection& right) noexcept
    {
        using std::swap;
        swap(left.impl_, right.impl_);
    }

    friend bool operator==(const connection& left,
                           const connection& right) noexcept
    {
        return left.impl_ == right.impl_;
    }

    friend bool operator<(const connection& left,
                          const connection& right) noexcept
    {
        return left.impl_ < right.impl_;
    }

private:
    friend connection
        make_connection(std::shared_ptr<detail::connection_info> ci) noexcept;
    // Private constructor - use by concrete instances of signal class
    connection(std::shared_ptr<detail::connection_info> c) noexcept
        : impl_{std::move(c)}
    {
        assert(impl_);
    }

private:
    std::shared_ptr<detail::connection_info> impl_;
};

inline connection
    make_connection(std::shared_ptr<detail::connection_info> ci) noexcept
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
    scoped_connection() noexcept = default;
    scoped_connection(connection connection) noexcept
        : connection_{std::move(connection)}
    {
    }

    ~scoped_connection() { connection_.disconnect(); }

    scoped_connection(const scoped_connection&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;

    scoped_connection(scoped_connection&& other) noexcept
        : connection_{std::move(other.connection_)}
    {
    }

    scoped_connection& operator=(scoped_connection&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    // Release underlying connection. Won't disconnect it upon destruction
    connection release() noexcept
    {
        connection ret{std::move(connection_)};
        return ret;
    }

    const connection& get() const noexcept { return connection_; }

    friend void swap(scoped_connection& left, scoped_connection& right) noexcept
    {
        using std::swap;
        swap(left.connection_, right.connection_);
    }

    friend bool operator==(const scoped_connection& left,
                           const scoped_connection& right) noexcept
    {
        return left.connection_ == right.connection_;
    }

    friend bool operator<(const scoped_connection& left,
                          const scoped_connection& right) noexcept
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
    template <typename Sink, typename Slot, typename... Args>
    static bool call(Sink&& sink, const Slot& slot, const Args&... args)
    {
        constexpr bool does_sink_return_void = std::is_same_v<
            void, std::invoke_result_t<Sink, Ret>>;

        auto&& return_value = slot(args...);

        if constexpr (does_sink_return_void)
            return std::forward<Sink>(sink)(return_value), false;
        else
            return !!std::forward<Sink>(sink)(return_value);
    }
};

template <>
struct sink_invoker<void>
{
    template <typename Sink, typename Slot, typename... Args>
    static bool call(Sink&& sink, const Slot& slot, const Args&... args)
    {
        constexpr bool does_sink_return_void =
            std::is_same_v<void, std::invoke_result_t<Sink>>;

        slot(args...);

        if constexpr (does_sink_return_void)
            return std::forward<Sink>(sink)(), false;
        else
            return !!std::forward<Sink>(sink)();
    }
};
} // namespace detail

template <typename Signature>
class signal;

template <typename Ret, typename... Args>
class signal<Ret(Args...)> : public detail::signal_base
{
public:
    using signature_type = Ret(Args...);
    using slot_type = std::function<Ret(Args...)>;
    using extended_slot_type = std::function<Ret(connection&, Args...)>;
    using signal_type = signal<Ret(Args...)>;
    using return_type = Ret;
    static const size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity, "invalid arg index");
        using type = std::tuple_element_t<N, std::tuple<Args...>>;
    };

    template <std::size_t N>
    using arg_t = typename arg<N>::type;

public:
    // Constructs empty, disconnected signal
    signal() noexcept = default;
    ~signal() { disconnect_all_slots(); }

    signal(const signal&) = delete;
    signal& operator=(const signal&) = delete;

    signal(signal&& other) noexcept
        : slots_{std::exchange(other.slots_, nullptr)},
          tail_{std::exchange(other.tail_, nullptr)},
          emits_{other.emits_}
    {
        rebind();
    }

    signal& operator=(signal&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    connection connect_extended(extended_slot_type extended_slot,
                                connect_position at = at_back)
    {
        if (!extended_slot)
            return {};

        class proxy_slot
        {
        public:
            explicit proxy_slot(const kl::connection& connection,
                                extended_slot_type slot)
                : connection_{connection}, slot_{std::move(slot)}
            {
            }

            return_type operator()(Args&&... args)
            {
                return slot_(connection_, std::forward<Args>(args)...);
            }

        private:
            kl::connection connection_;
            extended_slot_type slot_;
        };

        auto connection_info =
            std::make_shared<detail::connection_info>(this);
        // Create proxy slot that would add connection as a first argument
        auto connection = make_connection(connection_info);

        auto slot_info = new signal::slot(
            connection_info, proxy_slot{connection, std::move(extended_slot)});
        connection_info->id = reinterpret_cast<std::uintptr_t>(slot_info);
        insert_new_slot(slot_info, at);
        return connection;
    }

    // Connects signal object to given slot object.
    connection connect(slot_type slot, connect_position at = at_back)
    {
        if (!slot)
            return {};
        auto connection_info =
            std::make_shared<detail::connection_info>(this);
        auto slot_info = new signal::slot(connection_info, std::move(slot));
        connection_info->id = reinterpret_cast<std::uintptr_t>(slot_info);
        insert_new_slot(slot_info, at);
        return make_connection(std::move(connection_info));
    }

    connection operator+=(slot_type slot) { return connect(std::move(slot)); }

    // Emits signal
    void operator()(Args... args)
    {
        call_each_slot([&](const slot& s) {
            s(args...);
            return false;
        });

        cleanup_invalidated_slots();
    }

    // Emits signal and sinks signal return values
    template <typename Sink>
    void operator()(Args... args, Sink&& sink)
    {
        call_each_slot([&](const slot& s) {
            using invoker = detail::sink_invoker<return_type>;
            return invoker::call(std::forward<Sink>(sink), s, args...);
        });

        cleanup_invalidated_slots();
    }

    // Disconnects all slots bound to this signal
    void disconnect_all_slots() noexcept
    {
        for (auto iter = slots_; iter; iter = iter->next)
        {
            iter->invalidate();
            emits_ |= should_cleanup;
        }

        cleanup_invalidated_slots();
    }

    // Retrieves number of slots connected to this signal
    size_t num_slots() const noexcept
    {
        size_t sum{0};
        for (auto iter = slots_; iter; iter = iter->next)
        {
            if (iter->valid())
                ++sum;
        }
        return sum;
    }

    // Returns true if signal isn't connected to any slot
    bool empty() const noexcept { return num_slots() == 0; }

    friend void swap(signal& left, signal& right) noexcept
    {
        using std::swap;
        swap(left.slots_, right.slots_);
        swap(left.tail_, right.tail_);
        swap(left.emits_, right.emits_);

        left.rebind();
        right.rebind();
    }

private:
    void disconnect(std::uintptr_t id) override
    {
        for (auto iter = slots_; iter; iter = iter->next)
        {
            if (iter->id() != id)
                continue;

            // We can't delete the node here since we could be in the middle
            // of signal emission or we are called from extended slot. In
            // that case it would lead to removal of connection (proxy slot
            // keeps the copy) that is invoking this very function.
            iter->invalidate();
            emits_ |= should_cleanup;
            cleanup_invalidated_slots();
            return;
        }
    }

    void rebind() noexcept
    {
        cleanup_invalidated_slots_impl();

        // Rebind back-pointer to new signal
        for (auto iter = slots_; iter; iter = iter->next)
        {
            iter->rebind(this);
        }
    }

    template <typename Func>
    void call_each_slot(Func&& func)
    {
        struct decrement_op
        {
            unsigned& value;
            ~decrement_op() { --value; }
        } op{++emits_};

        // Save the last slot on the list to call. Any slots added after this
        // line during this emission (by reentrant call) will not be called.
        const auto last = tail_;

        for (auto iter = slots_; iter; iter = iter->next)
        {
            if (!iter->is_blocked() && iter->valid())
            {
                if (func(*iter))
                    break;
            }
            if (iter == last)
                break;
        }
    }

    void cleanup_invalidated_slots_impl()
    {
        assert(emits_ == 0 || emits_ == should_cleanup);

        for (slot *iter = slots_, *prev = nullptr; iter;)
        {
            if (!iter->valid())
            {
                auto next = iter->next;
                if (prev)
                    prev->next = next;
                if (slots_ == iter)
                    slots_ = next;
                if (tail_ == iter)
                    tail_ = prev ? prev : slots_;
                delete iter;
                iter = next;
            }
            else
            {
                prev = iter;
                iter = iter->next;
            }
        }
    }

    void cleanup_invalidated_slots()
    {
        // We can free dead slots only when we're not in the middle of an
        // emission and also there's anything to cleanup
        if (emits_ == should_cleanup)
        {
            cleanup_invalidated_slots_impl();
            emits_ = 0;
        }
    }

private:
    class slot final
    {
    public:
        slot* next{nullptr};

    public:
        slot(std::shared_ptr<detail::connection_info> connection_info,
             slot_type impl) noexcept
            : connection_info_{std::move(connection_info)},
              target_{std::move(impl)}

        {
        }

        slot(const slot&) = delete;
        slot& operator=(const slot&) = delete;

        return_type operator()(const Args&... args) const
        {
            return target_(args...);
        }

        std::uintptr_t id() const noexcept { return connection_info_->id; }
        void invalidate() noexcept { rebind(nullptr); }

        void rebind(signal_base* self) noexcept
        {
            connection_info_->sender = self;
        }

        bool valid() const noexcept
        {
            return connection_info_->sender != nullptr;
        }
        bool is_blocked() const noexcept
        {
            return connection_info_->blocking > 0;
        }

    private:
        std::shared_ptr<detail::connection_info> connection_info_;
        slot_type target_;
    };

    slot* slots_{nullptr}; // owning pointer
    slot* tail_{nullptr};  // observer ptr

    // On the most significant bit we store if there are any invalidated slots
    // that need to be cleanup.
    unsigned emits_{0};
    // If emits_ equals to should_cleanup we know there's no emission in place
    // and there are invalidated slots.
    static constexpr unsigned should_cleanup = 0x80000000;

private:
    void insert_new_slot(slot* new_slot, connect_position at) noexcept
    {
        if (!slots_)
        {
            slots_ = new_slot;
            tail_ = new_slot;
            return;
        }

        if (at == at_back)
        {
            auto iter = slots_, prev = slots_;
            while (iter)
            {
                prev = iter;
                iter = iter->next;
            }

            iter = prev;
            iter->next = new_slot;
            tail_ = new_slot;
        }
        else
        {
            new_slot->next = slots_;
            slots_ = new_slot;
        }
    }
};

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...), T* instance)
{
    return {[mem_func, instance](Args&&... args) {
        return (instance->*mem_func)(std::forward<Args>(args)...);
    }};
}

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...) const,
                                      const T* instance)
{
    return {[mem_func, instance](Args&&... args) {
        return (instance->*mem_func)(std::forward<Args>(args)...);
    }};
}

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...), T& instance)
{
    return {[mem_func, &instance](Args&&... args) {
        return (instance.*mem_func)(std::forward<Args>(args)...);
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

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...) const,
                                      std::shared_ptr<T> instance)
{
    return {[mem_func, instance](Args&&... args) {
        return ((*instance).*mem_func)(std::forward<Args>(args)...);
    }};
}

template <typename T, typename Ret, typename... Args>
std::function<Ret(Args...)> make_slot(Ret (T::*mem_func)(Args...) const,
                                      std::weak_ptr<T> instance)
{
    return {[mem_func, instance](Args&&... args) {
        if (auto ptr = instance.lock())
            return ((*ptr).*mem_func)(std::forward<Args>(args)...);
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
    size_t operator()(const kl::connection& key) const noexcept
    {
        return key.hash();
    }
};

template <>
struct hash<kl::scoped_connection>
{
    size_t operator()(const kl::scoped_connection& key) const noexcept
    {
        return hash<kl::connection>{}(key.get());
    }
};
} // namespace std

// Handy macro for slot connecting inside a class
#define KL_SLOT(mem_fn)                                                        \
    [this](auto&&... args) {                                                   \
        return this->mem_fn(std::forward<decltype(args)>(args)...);            \
    }
