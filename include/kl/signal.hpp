#pragma once

#include <kl/defer.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace kl::detail {

struct signal_base
{
    virtual void disconnect(std::uintptr_t id) noexcept = 0;

protected:
    ~signal_base() = default;
};

struct slot_state final
{
    slot_state(signal_base* sender) noexcept
        : sender{sender}
    {
    }

    signal_base* sender;
    std::uintptr_t id{0};
    unsigned blocking{0};
};

struct tls_signal_info
{
    bool emission_stopped{false};
    const std::shared_ptr<detail::slot_state>* current_slot{nullptr};
};

inline auto& get_tls_signal_info() noexcept
{
    thread_local tls_signal_info info;
    return info;
}
} // namespace kl::detail

namespace kl {

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
        : state_{std::exchange(other.state_, nullptr)}
    {
    }
    connection& operator=(connection&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    // Disconnects the connection. No-op if connection is already disconnected.
    void disconnect() noexcept
    {
        if (!connected())
            return;
        state_->sender->disconnect(state_->id);
        state_ = nullptr;
    }

    // Returns true if connection is valid
    bool connected() const noexcept { return state_ && state_->sender; }

    size_t hash() const noexcept
    {
        // Get hash out of underlying pointer
        return std::hash<std::shared_ptr<detail::slot_state>>{}(state_);
    }

    class blocker
    {
    public:
        blocker() noexcept = default;
        blocker(connection& con) noexcept: state_{con.state_}
        {
            if (state_)
                ++state_->blocking;
        }

        ~blocker()
        {
            if (state_)
                --state_->blocking;
        }

        blocker(const blocker& other) noexcept : state_{other.state_}
        {
            if (state_)
                ++state_->blocking;
        }

        blocker& operator=(const blocker& other) noexcept
        {
            if (this != &other)
            {
                if (state_)
                    --state_->blocking;
                state_ = other.state_;
                if (state_)
                    ++state_->blocking;
            }
            return *this;
        }

        blocker(blocker&& other) noexcept : state_{std::move(other.state_)} {}

        blocker& operator=(blocker&& other) noexcept
        {
            swap(*this, other);
            return *this;
        }

        bool blocking() const noexcept { return state_ != nullptr; }

        friend void swap(blocker& left, blocker& right) noexcept
        {
            using std::swap;
            swap(left.state_, right.state_);
        }

    private:
        std::shared_ptr<detail::slot_state> state_;
    };

    blocker get_blocker() noexcept { return {*this}; }

public:
    friend void swap(connection& left, connection& right) noexcept
    {
        using std::swap;
        swap(left.state_, right.state_);
    }

    friend bool operator==(const connection& left,
                           const connection& right) noexcept
    {
        return left.state_ == right.state_;
    }

    friend bool operator<(const connection& left,
                          const connection& right) noexcept
    {
        return left.state_ < right.state_;
    }

private:
    friend connection
        make_connection(std::shared_ptr<detail::slot_state> state) noexcept;
    // Private constructor - use by concrete instances of signal class
    connection(std::shared_ptr<detail::slot_state> state) noexcept
        : state_{std::move(state)}
    {
        assert(state_);
    }

private:
    std::shared_ptr<detail::slot_state> state_;
};

inline connection
    make_connection(std::shared_ptr<detail::slot_state> state) noexcept
{
    return connection(std::move(state));
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

template <typename Signature>
class signal;

template <typename... Args>
class signal<void(Args...)> : public detail::signal_base
{
public:
    using signature_type = void(Args...);
    using slot_type = std::function<void(Args...)>;

public:
    // Constructs empty, disconnected signal
    signal() noexcept = default;
    ~signal() { disconnect_all_slots(); }

    signal(const signal&) = delete;
    signal& operator=(const signal&) = delete;

    signal(signal&& other) noexcept
        : slots_{std::exchange(other.slots_, nullptr)},
          tail_{std::exchange(other.tail_, nullptr)},
          deferred_cleanup_{other.deferred_cleanup_}
    {
        rebind();
    }

    signal& operator=(signal&& other) noexcept
    {
        if (this != &other)
            swap(*this, other);
        return *this;
    }

    connection connect(slot_type slot, connect_position at = at_back)
    {
        if (!slot)
            return {};
        auto slot_state =
            std::make_shared<detail::slot_state>(this);
        auto slot_impl = new signal::slot(slot_state, std::move(slot));
        slot_state->id = reinterpret_cast<std::uintptr_t>(slot_impl);
        insert_new_slot(slot_impl, at);
        return make_connection(std::move(slot_state));
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...), T* instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, instance](Args&&... args) {
                (instance->*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...) const, const T* instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, instance](Args&&... args) {
                (instance->*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...), T& instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, &instance](Args&&... args) {
                (instance.*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...) const, const T& instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, &instance](Args&&... args) {
                (instance.*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...), std::shared_ptr<T> instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, instance](Args&&... args) {
                ((*instance).*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...) const,
                       std::shared_ptr<const T> instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, instance](Args&&... args) {
                ((*instance).*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...), std::weak_ptr<T> instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, instance](Args&&... args) {
                if (auto ptr = instance.lock())
                    ((*ptr).*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename T, typename Ret, typename... Args2>
    connection connect(Ret (T::*mem_func)(Args2...) const,
                       std::weak_ptr<const T> instance,
                       connect_position at = at_back)
    {
        return connect(
            [mem_func, instance](Args&&... args) {
                if (auto ptr = instance.lock())
                    ((*ptr).*mem_func)(std::forward<Args>(args)...);
            },
            at);
    }

    template <typename Ret, typename... Args2>
    connection connect(signal<Ret(Args2...)>& sig, connect_position at = at_back)
    {
        return connect(
            [&sig](Args&&... args) { sig(std::forward<Args>(args)...); }, at);
    }

    connection operator+=(slot_type slot)
    {
        return connect(std::move(slot));
    }

    // Emits signal
    void operator()(Args... args)
    {
        auto& emission_state = detail::get_tls_signal_info();
        // Rollback tls_signal_info back to its pre-emission value
        auto prev_emission_state = emission_state;
        KL_DEFER(emission_state = prev_emission_state);

        emission_state.emission_stopped = false;

        // Save the last slot on the list to call. Any slots added after this
        // line during this emission (by reentrant call) will not be called.
        const auto last = tail_;

        for (auto iter = slots_; iter; iter = iter->next)
        {
            if (!iter->is_blocked() && iter->valid())
            {
                ++iter->used;
                KL_DEFER(--iter->used);
                emission_state.current_slot = &iter->state();

                (*iter)(args...);

                if (emission_state.emission_stopped)
                    break;
            }
            // Last represents last item on the list, inclusively so we need to
            // stop iterating after handling it, not just before.
            if (iter == last)
                break;
        }

        cleanup_invalidated_slots();
    }

    // Disconnects all slots bound to this signal
    void disconnect_all_slots() noexcept
    {
        for (slot *iter = slots_, *prev = nullptr; iter;)
        {
            if (iter->invalidate())
            {
                iter = remove_slot(*iter, prev);
            }
            else
            {
                deferred_cleanup_ = true;
                prev = iter;
                iter = iter->next;
            }
        }
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
        swap(left.deferred_cleanup_, right.deferred_cleanup_);

        left.rebind();
        right.rebind();
    }

private:
    void disconnect(std::uintptr_t id) noexcept override
    {
        for (slot *iter = slots_, *prev = nullptr; iter;
             prev = iter, iter = iter->next)
        {
            if (iter->id() != id)
                continue;

            if (iter->invalidate())
            {
                // No one uses the slot right now so we can safely remove it.
                remove_slot(*iter, prev);
            }
            else
            {
                // We'll try again after all emissions are finished
                deferred_cleanup_ = true;
            }
            return;
        }
    }

    void rebind() noexcept
    {
        cleanup_invalidated_slots();

        // Rebind back-pointer to new signal
        for (auto iter = slots_; iter; iter = iter->next)
        {
            iter->rebind(this);
        }
    }

    void cleanup_invalidated_slots() noexcept
    {
        if (!deferred_cleanup_)
            return;

        bool new_deferred_cleanup_ = false;
        for (slot *iter = slots_, *prev = nullptr; iter;)
        {
            const bool is_valid = iter->valid();

            if (is_valid || iter->used > 0)
            {
                if (!is_valid) // iter->used > 0
                    new_deferred_cleanup_ = true;
                prev = iter;
                iter = iter->next;
            }
            else
            {
                iter = remove_slot(*iter, prev);
            }
        }
        deferred_cleanup_ = new_deferred_cleanup_;
    }

private:
    struct slot final
    {
        slot(std::shared_ptr<detail::slot_state> slot_state,
             slot_type target) noexcept
            : state_{std::move(slot_state)},
              target_{std::move(target)}
        {
        }

        slot(const slot&) = delete;
        slot& operator=(const slot&) = delete;

        void operator()(const Args&... args) const
        {
            target_(args...);
        }

        const std::shared_ptr<detail::slot_state>& state() { return state_; }
        std::uintptr_t id() const noexcept { return state_->id; }

        bool invalidate() noexcept
        {
            rebind(nullptr);
            return used == 0;
        }
        void rebind(signal_base* self) noexcept { state_->sender = self; }
        bool valid() const noexcept { return state_->sender != nullptr; }

        bool is_blocked() const noexcept { return state_->blocking > 0; }

    public:
        slot* next{nullptr};
        std::uint32_t used{0};
    private:
        const std::shared_ptr<detail::slot_state> state_;
        const slot_type target_;
    };

    slot* slots_{nullptr}; // owning pointer
    slot* tail_{nullptr};  // observer ptr
    bool deferred_cleanup_{false};

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

    slot* remove_slot(slot& to_remove, slot* prev) noexcept
    {
        assert(to_remove.used == 0);
        slot* next = to_remove.next;
        if (prev)
            prev->next = next;
        else if (slots_ == &to_remove)
            slots_ = next;
        if (tail_ == &to_remove)
            tail_ = prev ? prev : slots_;
        delete &to_remove;
        return next;
    }
};


} // namespace kl

namespace kl::this_signal {

void stop_emission() noexcept
{
    detail::get_tls_signal_info().emission_stopped = true;
}

connection current_connection() noexcept
{
    auto state = detail::get_tls_signal_info().current_slot;
    return state ? make_connection(*state) : connection{};
}
} // namespace kl::this_signal

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
