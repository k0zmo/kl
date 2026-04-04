#pragma once

#include <kl/defer.hpp>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace kl::detail {

struct signal_base
{
    virtual void disconnect(std::uintptr_t id) noexcept = 0;

protected:
    ~signal_base() = default;
};

template <typename Derived>
struct ref_counted
{
public:
    ref_counted() noexcept : count_{0} {}
    ref_counted(const ref_counted&) noexcept : count_{0} {}
    ref_counted& operator=(const ref_counted&) noexcept { return *this; }

    void add_ref() noexcept { count_.fetch_add(1, std::memory_order_relaxed); }
    void release() noexcept
    {
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete static_cast<Derived*>(this);
    }

private:
    std::atomic<std::ptrdiff_t> count_;
};

class slot_state : public ref_counted<slot_state>
{
public:
    explicit slot_state(signal_base* sender) noexcept
        : sender_{sender}
    {
        add_ref();
    }
    virtual ~slot_state() = default;

    slot_state(const slot_state&) = delete;
    slot_state& operator=(const slot_state&) = delete;

    void block() noexcept
    {
        blocking_.fetch_add(1, std::memory_order_relaxed);
    }

    void unblock() noexcept
    {
        const auto prev = blocking_.fetch_sub(1, std::memory_order_relaxed);
        assert(prev > 0);
        (void)prev;
    }

    bool blocked() const noexcept
    {
        return blocking_.load(std::memory_order_relaxed) > 0;
    }

    bool connected() const noexcept
    {
        return sender_.load(std::memory_order_acquire) != nullptr;
    }

    void disconnect() noexcept
    {
        auto* sender = sender_.exchange(nullptr, std::memory_order_acq_rel);
        if (sender)
            sender->disconnect(id());
    }

    bool valid() const noexcept
    {
        return sender_.load(std::memory_order_acquire) != nullptr;
    }

    std::uintptr_t id() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(this);
    }

protected:
    std::atomic<signal_base*> sender_;

private:
    std::atomic<unsigned> blocking_{0};
};

struct tls_signal_info
{
    bool emission_stopped{false};
    detail::slot_state* current_slot{nullptr};
};

inline auto& get_tls_signal_info() noexcept
{
    thread_local tls_signal_info info;
    return info;
}
} // namespace kl::detail

namespace kl {

/**
 * @brief Handle to a slot connected to a signal.
 *
 * A connection can be copied, moved, stored in standard containers and used to
 * disconnect the underlying slot later.
 */
class connection final
{
public:
    connection() noexcept : slot_{} {}
    explicit connection(detail::slot_state& slot) noexcept : slot_{&slot}
    {
        slot_->add_ref();
    }

    ~connection()
    {
        if (slot_)
            slot_->release();
    }

    connection(const connection& other) : slot_{other.slot_}
    {
        if (slot_)
            slot_->add_ref();
    }

    connection& operator=(const connection& other)
    {
        if (this != &other)
        {
            if (slot_)
                slot_->release();
            slot_ = other.slot_;
            if (slot_)
                slot_->add_ref();
        }
        return *this;
    }

    connection(connection&& other) noexcept
        : slot_{std::exchange(other.slot_, nullptr)}
    {
    }

    connection& operator=(connection&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    /**
     * @brief Disconnects the underlying slot.
     *
     * Does nothing if the connection is already empty or disconnected.
     */
    void disconnect() noexcept
    {
        if (!slot_)
            return;
        slot_->disconnect();
        slot_->release();
        slot_ = nullptr;
    }

    /// Returns whether the connection still refers to a connected slot.
    bool connected() const noexcept { return slot_ && slot_->connected(); }

    /// Returns a hash value based on the underlying slot identity.
    size_t hash() const noexcept
    {
        return std::hash<detail::slot_state*>{}(slot_);
    }

public:
    /**
     * @brief RAII blocker for a connection.
     *
     * While a blocker is alive, the associated slot remains connected but is
     * skipped during signal emission.
     */
    class blocker
    {
    public:
        blocker() noexcept = default;
        blocker(connection& con) noexcept: slot_{con.slot_}
        {
            if (slot_)
            {
                slot_->add_ref();
                slot_->block();
            }
        }

        ~blocker()
        {
            if (slot_)
            {
                slot_->unblock();
                slot_->release();
            }
        }

        blocker(const blocker& other) noexcept : slot_{other.slot_}
        {
            if (slot_)
            {
                slot_->add_ref();
                slot_->block();
            }
        }

        blocker& operator=(const blocker& other) noexcept
        {
            if (this != &other)
            {
                if (slot_)
                {
                    slot_->unblock();
                    slot_->release();
                }
                slot_ = other.slot_;
                if (slot_)
                {
                    slot_->add_ref();
                    slot_->block();
                }
            }
            return *this;
        }

        blocker(blocker&& other) noexcept
            : slot_{std::exchange(other.slot_, nullptr)}
        {
        }

        blocker& operator=(blocker&& other) noexcept
        {
            swap(*this, other);
            return *this;
        }

        /// Returns whether this blocker currently suppresses a slot.
        bool blocking() const noexcept { return slot_ != nullptr; }

        friend void swap(blocker& left, blocker& right) noexcept
        {
            using std::swap;
            swap(left.slot_, right.slot_);
        }

    private:
        detail::slot_state* slot_{};
    };

    /// Creates an RAII blocker for this connection.
    blocker get_blocker() noexcept { return {*this}; }

public:
    friend void swap(connection& left, connection& right) noexcept
    {
        using std::swap;
        swap(left.slot_, right.slot_);
    }

    friend bool operator==(const connection& left,
                           const connection& right) noexcept
    {
        return left.slot_ == right.slot_;
    }

    friend bool operator<(const connection& left,
                          const connection& right) noexcept
    {
        return left.slot_ < right.slot_;
    }

private:
    detail::slot_state* slot_;
};

/**
 * @brief RAII wrapper around @ref connection.
 *
 * A scoped connection disconnects the underlying slot when the wrapper is
 * destroyed unless ownership has been released.
 */
class scoped_connection final
{
public:
    /// Constructs an empty scoped connection.
    scoped_connection() noexcept = default;

    /// Takes ownership of an existing connection.
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

    /**
     * @brief Releases ownership of the underlying connection.
     *
     * After this call the scoped connection becomes empty and will no longer
     * disconnect the slot on destruction.
     */
    connection release() noexcept
    {
        connection ret{std::move(connection_)};
        return ret;
    }

    /// Returns the underlying connection.
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

/// Controls whether newly connected slots are inserted at the front or back.
enum connect_position
{
    /// Append the new slot after existing slots.
    at_back,
    /// Insert the new slot before existing slots.
    at_front
};

template <typename Signature>
class signal;

/**
 * @brief Signal type for the given function signature.
 *
 * Slots may be connected, disconnected or blocked while an emission is in
 * progress. Changes made during emission affect future emissions, while the
 * currently running emission continues using the traversal rules implemented by
 * this class.
 *
 * Thread safety:
 * Concurrent emission and connect/disconnect operations are supported.
 *
 * Limitation:
 * A signal object must not be moved or swapped while any emission is in
 * progress on that object. Violating this precondition triggers an assertion in
 * debug builds and terminates the process in all builds.
 */
template <typename... Args>
class signal<void(Args...)> final : public detail::signal_base
{
public:
    /// Signal function signature.
    using signature_type = void(Args...);
    /// Type-erased callable type used for slots.
    using slot_type = std::function<void(Args...)>;

public:
    /// Constructs empty, disconnected signal.
    signal() noexcept = default;
    ~signal() { disconnect_all_slots(); }

    signal(const signal&) = delete;
    signal& operator=(const signal&) = delete;

    /**
     * @brief Move-constructs a signal.
     *
     * @warning Moving a signal while it is being emitted is not allowed.
     * The program terminates if this precondition is violated.
     */
    signal(signal&& other) noexcept
    {
        {
            std::lock_guard<std::mutex> lock{other.mutex_};
            terminate_if_emitting_locked(other.emission_in_progress());
            slots_ = std::exchange(other.slots_, nullptr);
            tail_ = std::exchange(other.tail_, nullptr);
            deferred_cleanup_ = other.deferred_cleanup_;
            other.deferred_cleanup_ = false;
        }
        std::lock_guard<std::mutex> lock{mutex_};
        rebind_locked();
    }

    /**
     * @brief Move-assigns a signal.
     *
     * @warning Moving or swapping a signal while either operand is being
     * emitted is not allowed. The program terminates if this precondition is
     * violated.
     */
    signal& operator=(signal&& other) noexcept
    {
        if (this != &other)
            swap(*this, other);
        return *this;
    }

    /**
     * @brief Connects a callable slot.
     *
     * @param slot Callable to invoke on emission.
     * @param at Position at which the slot is inserted.
     * @return A connection that can later disconnect or block the slot.
     */
    connection connect(slot_type slot, connect_position at = at_back)
    {
        if (!slot)
            return {};
        std::lock_guard<std::mutex> lock{mutex_};
        auto slot_impl = new signal::slot(this, std::move(slot));
        insert_new_slot_locked(slot_impl, at);
        return connection{*slot_impl};
    }

    /// Connects a member function and raw object pointer.
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

    /// Connects a const member function and raw object pointer.
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

    /// Connects a member function and object reference.
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

    /// Connects a const member function and object reference.
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

    /// Connects a member function and shared object instance.
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

    /// Connects a const member function and shared const object instance.
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

    /// Connects a member function and weak object instance.
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

    /// Connects a const member function and weak const object instance.
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

    /// Connects another signal so each emission forwards to it.
    template <typename Ret, typename... Args2>
    connection connect(signal<Ret(Args2...)>& sig, connect_position at = at_back)
    {
        return connect(
            [&sig](Args&&... args) { sig(std::forward<Args>(args)...); }, at);
    }

    /// Convenience shorthand for @ref connect(slot_type, connect_position).
    connection operator+=(slot_type slot)
    {
        return connect(std::move(slot));
    }

    /**
     * @brief Emits the signal to connected slots.
     *
     * Slots connected after emission starts are not visited by that in-flight
     * emission. Slots disconnected during emission are skipped once they become
     * invalid.
     */
    void operator()(Args... args)
    {
        active_emissions_.fetch_add(1, std::memory_order_acq_rel);
        KL_DEFER({
            const auto prev = active_emissions_.fetch_sub(1, std::memory_order_acq_rel);
            if (prev == 1)
            {
                std::lock_guard<std::mutex> lock{mutex_};
                cleanup_invalidated_slots_locked();
            }
        });

        auto& emission_state = detail::get_tls_signal_info();
        // Rollback tls_signal_info back to its pre-emission value
        auto prev_emission_state = emission_state;
        KL_DEFER(emission_state = prev_emission_state);

        emission_state.emission_stopped = false;

        // We can't traverse the list without synchronization while other
        // threads may connect/disconnect. Instead we:
        //  - snapshot the tail ("last") once under lock
        //  - traverse slot-by-slot, acquiring the lock only to read next
        //  - hold a ref to each slot while invoking user code
        slot* last = nullptr;
        slot* current = nullptr;
        {
            std::lock_guard<std::mutex> lock{mutex_};
            last = tail_;
            if (last)
                last->add_ref();
            current = slots_;
            if (current)
                current->add_ref();
        }
        KL_DEFER(
            if (last)
                last->release();
        );

        while (current)
        {
            if (!current->blocked() && current->valid())
            {
                // Invoke the slot if it's valid (not disconnected) and not blocked
                emission_state.current_slot = current;
                current->target(args...);
            }

            const bool is_last = (current == last);

            slot* next = nullptr;
            if (!is_last && !emission_state.emission_stopped)
            {
                std::lock_guard<std::mutex> lock{mutex_};
                next = current->next;
                if (next)
                    next->add_ref();
            }

            current->release();
            current = next;

            if (is_last || emission_state.emission_stopped)
                break;
        }
    }

    /**
     * @brief Disconnects every slot currently owned by the signal.
     *
     * If called during emission, disconnected slots are invalidated
     * immediately and physically removed after the emission completes.
     */
    void disconnect_all_slots() noexcept
    {
        std::lock_guard<std::mutex> lock{mutex_};
        const bool can_remove = !emission_in_progress();
        for (slot *iter = slots_, *prev = nullptr; iter;)
        {
            iter->invalidate();
            if (can_remove)
            {
                iter = remove_slot_locked(*iter, prev);
            }
            else
            {
                deferred_cleanup_ = true;
                prev = iter;
                iter = iter->next;
            }
        }
    }

    /// Returns the number of currently connected slots.
    size_t num_slots() const noexcept
    {
        std::lock_guard<std::mutex> lock{mutex_};
        size_t sum{0};
        for (auto iter = slots_; iter; iter = iter->next)
        {
            if (iter->valid())
                ++sum;
        }
        return sum;
    }

    /// Returns whether the signal has no connected slots.
    bool empty() const noexcept { return num_slots() == 0; }

    /**
     * @brief Swaps two signals.
     *
     * @warning Swapping while either signal is being emitted is not allowed.
     * The program terminates if this precondition is violated.
     */
    friend void swap(signal& left, signal& right) noexcept
    {
        if (&left == &right)
            return;
        std::scoped_lock lock{left.mutex_, right.mutex_};
        terminate_if_emitting_locked(left.emission_in_progress() ||
                                     right.emission_in_progress());
        using std::swap;
        swap(left.slots_, right.slots_);
        swap(left.tail_, right.tail_);
        swap(left.deferred_cleanup_, right.deferred_cleanup_);

        left.rebind_locked();
        right.rebind_locked();
    }

private:
    static void terminate_if_emitting_locked(bool emission_in_progress) noexcept
    {
        if (!emission_in_progress)
            return;
        assert(!emission_in_progress &&
               "moving or swapping a signal during emission is not allowed");
        std::terminate();
    }

    void disconnect(std::uintptr_t id) noexcept override
    {
        std::lock_guard<std::mutex> lock{mutex_};
        const bool can_remove = !emission_in_progress();
        for (slot *iter = slots_, *prev = nullptr; iter;
             prev = iter, iter = iter->next)
        {
            if (iter->id() != id)
                continue;

            iter->invalidate();
            if (can_remove)
            {
                remove_slot_locked(*iter, prev);
            }
            else
            {
                // We'll try again after all emissions are finished
                deferred_cleanup_ = true;
            }
            return;
        }
    }

    void rebind_locked() noexcept
    {
        cleanup_invalidated_slots_locked();

        // Rebind back-pointer to new signal
        for (auto iter = slots_; iter; iter = iter->next)
        {
            iter->rebind(this);
        }
    }

    void cleanup_invalidated_slots_locked() noexcept
    {
        if (!deferred_cleanup_)
            return;

        // Never physically remove slots while any emission is in-flight.
        if (emission_in_progress())
        {
            deferred_cleanup_ = true;
            return;
        }

        for (slot *iter = slots_, *prev = nullptr; iter;)
        {
            if (iter->valid())
            {
                prev = iter;
                iter = iter->next;
            }
            else
            {
                iter = remove_slot_locked(*iter, prev);
            }
        }
        deferred_cleanup_ = false;
    }

private:
    struct slot final : detail::slot_state
    {
        slot(signal_base* parent, slot_type target) noexcept
            : detail::slot_state{parent},
              target{std::move(target)}
        {
        }

        slot(const slot&) = delete;
        slot& operator=(const slot&) = delete;

        void invalidate() noexcept
        {
            rebind(nullptr);
        }

        void rebind(signal_base* parent) noexcept
        {
            sender_.store(parent, std::memory_order_release);
        }

        slot* next{nullptr};
        const slot_type target;
    };

    slot* slots_{nullptr}; // owning pointer
    slot* tail_{nullptr};  // observer ptr
    std::atomic<std::uint32_t> active_emissions_{0};
    mutable std::mutex mutex_;
    bool deferred_cleanup_{false};

private:
    bool emission_in_progress() const noexcept
    {
        return active_emissions_.load(std::memory_order_acquire) != 0;
    }

    void insert_new_slot_locked(slot* new_slot, connect_position at) noexcept
    {
        if (!slots_)
        {
            slots_ = new_slot;
            tail_ = new_slot;
            return;
        }

        if (at == at_back)
        {
            tail_->next = new_slot;
            tail_ = new_slot;
        }
        else
        {
            new_slot->next = slots_;
            slots_ = new_slot;
        }
    }

    slot* remove_slot_locked(slot& to_remove, slot* prev) noexcept
    {
        assert(!emission_in_progress());
        slot* next = to_remove.next;
        if (prev)
            prev->next = next;
        else if (slots_ == &to_remove)
            slots_ = next;
        if (tail_ == &to_remove)
            tail_ = prev ? prev : slots_;
        to_remove.release();
        return next;
    }
};

} // namespace kl

namespace kl::this_signal {

/**
 * @brief Stops the current signal emission on the calling thread.
 *
 * After the currently executing slot returns, no further slots from the same
 * in-flight emission are invoked.
 */
inline void stop_emission() noexcept
{
    detail::get_tls_signal_info().emission_stopped = true;
}

/**
 * @brief Returns a connection to the slot currently being invoked.
 *
 * The returned connection is empty when called outside of a signal handler.
 */
inline connection current_connection() noexcept
{
    auto slot = detail::get_tls_signal_info().current_slot;
    return slot ? connection{*slot} : connection{};
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
