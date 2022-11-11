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

template <typename Derived>
struct ref_counted
{
public:
    ref_counted() noexcept : count_{0} {}
    ref_counted(const ref_counted&) noexcept : count_{0} {}
    ref_counted& operator=(const ref_counted&) noexcept { return *this; }

    void add_ref() noexcept { ++count_; }
    void release() noexcept
    {
        if (--count_ == 0)
            delete static_cast<Derived*>(this);
    }

private:
    std::ptrdiff_t count_;
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
        ++blocking_;
    }

    void unblock() noexcept
    {
        assert(blocking_ > 0);
        --blocking_;
    }

    bool blocked() const noexcept
    {
        return blocking_ > 0;
    }

    bool connected() const noexcept
    {
        return sender_ != nullptr;
    }

    void disconnect() noexcept
    {
        if (sender_)
        {
            sender_->disconnect(id());
            sender_ = nullptr;
        }
    }

    bool valid() const noexcept
    {
        return sender_ != nullptr;
    }

    std::uintptr_t id() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(this);
    }

protected:
    signal_base* sender_;

private:
    unsigned blocking_{0};
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

/*
 * Represents a connection from given signal to a slot. Can be used to disconnect
 * the connection at any time later.
 * connection object can be kept inside standard containers (e.g unordered_set)
 * compared between each others and finally move/copy around.
 */
class connection final
{
public:
    connection() noexcept : slot_{} {}

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

    // Disconnects the connection. No-op if connection is already disconnected.
    void disconnect() noexcept
    {
        if (!slot_)
            return;
        slot_->disconnect();
        slot_->release();
        slot_ = nullptr;
    }

    // Returns true if connection is valid
    bool connected() const noexcept { return slot_ && slot_->connected(); }

    size_t hash() const noexcept
    {
        // Get hash out of underlying pointer
        return std::hash<detail::slot_state*>{}(slot_);
    }

public:
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

        bool blocking() const noexcept { return slot_ != nullptr; }

        friend void swap(blocker& left, blocker& right) noexcept
        {
            using std::swap;
            swap(left.slot_, right.slot_);
        }

    private:
        detail::slot_state* slot_{};
    };
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
    friend connection make_connection(detail::slot_state& state) noexcept;
    explicit connection(detail::slot_state& slot) noexcept : slot_{&slot}
    {
        slot_->add_ref();
    }

private:
    detail::slot_state* slot_;
};

inline connection make_connection(detail::slot_state& state) noexcept
{
    return connection(state);
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
        auto slot_impl = new signal::slot(this, std::move(slot));
        insert_new_slot(slot_impl, at);
        return make_connection(*slot_impl);
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
            if (!iter->blocked() && iter->valid())
            {
                ++iter->num_emissions;
                KL_DEFER(--iter->num_emissions);
                emission_state.current_slot = iter;

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

            if (is_valid || iter->num_emissions > 0)
            {
                if (!is_valid) // iter->num_emissions > 0
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
    struct slot final : detail::slot_state
    {
        slot(signal_base* parent, slot_type target) noexcept
            : detail::slot_state{parent},
              target_{std::move(target)}
        {
        }

        slot(const slot&) = delete;
        slot& operator=(const slot&) = delete;

        void operator()(const Args&... args) const
        {
            target_(args...);
        }

        bool invalidate() noexcept
        {
            rebind(nullptr);
            return num_emissions == 0;
        }

        void rebind(signal_base* parent) noexcept
        {
            sender_ = parent;
        }

    public:
        slot* next{nullptr};
        std::uint32_t num_emissions{0};
    private:
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
        assert(to_remove.num_emissions == 0);
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

void stop_emission() noexcept
{
    detail::get_tls_signal_info().emission_stopped = true;
}

connection current_connection() noexcept
{
    auto slot = detail::get_tls_signal_info().current_slot;
    return slot ? make_connection(*slot) : connection{};
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
