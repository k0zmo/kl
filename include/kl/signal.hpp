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
    std::shared_ptr<detail::slot_state>* slot_state{nullptr};
};

inline auto& get_tls_signal_info()
{
    thread_local static tls_signal_info info;
    return info;
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
        : state_{std::exchange(other.state_, nullptr)}
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
        state_->sender->disconnect(state_->id);
        state_ = nullptr;
    }

    // Returns true if connection is valid
    bool connected() const { return state_ && state_->sender; }

    size_t hash() const noexcept
    {
        // Get hash out of underlying pointer
        return std::hash<std::shared_ptr<detail::slot_state>>{}(state_);
    }

    class blocker
    {
    public:
        blocker() noexcept = default;
        blocker(connection& con) : state_{con.state_}
        {
            if (state_)
                ++state_->blocking;
        }

        ~blocker()
        {
            if (state_)
                --state_->blocking;
        }

        blocker(const blocker& other) : state_{other.state_}
        {
            if (state_)
                ++state_->blocking;
        }

        blocker& operator=(const blocker& other)
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

    blocker get_blocker() { return {*this}; }

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
    using signal_type = signal<void(Args...)>;
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

    // Connects signal object to given slot object.
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

    // Converts signal to slot type allowing to create signal-to-signal
    // connections
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
        auto& tls = detail::get_tls_signal_info();
        struct scoped_emission
        {
            scoped_emission(detail::tls_signal_info& info, unsigned& num_emits)
                : info(info),
                  prev_value{info},
                  num_emits{++num_emits}
            {
                info.emission_stopped = false;
            }
            ~scoped_emission()
            {
                --num_emits;
                info = prev_value;
            }

            detail::tls_signal_info& info;
            detail::tls_signal_info prev_value;
            unsigned& num_emits;
        } emission{tls, emits_};

        // Save the last slot on the list to call. Any slots added after this
        // line during this emission (by reentrant call) will not be called.
        const auto last = tail_;

        for (auto iter = slots_; iter; iter = iter->next)
        {
            if (!iter->is_blocked() && iter->valid())
            {
                emission.info.slot_state = &iter->state_ref();

                (*iter)(args...);

                if (tls.emission_stopped)
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
            // of signal emission. In that case it would lead to removal of
            // connection that is invoking this very function.
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

        std::shared_ptr<detail::slot_state>& state_ref() { return state_; }
        std::uintptr_t id() const noexcept { return state_->id; }

        void invalidate() noexcept { rebind(nullptr); }
        void rebind(signal_base* self) noexcept { state_->sender = self; }
        bool valid() const noexcept { return state_->sender != nullptr; }

        bool is_blocked() const noexcept { return state_->blocking > 0; }

    private:
        std::shared_ptr<detail::slot_state> state_;
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

namespace this_signal {

void stop_emission()
{
    detail::get_tls_signal_info().emission_stopped = true;
}

connection current_connection() {

    auto state = detail::get_tls_signal_info().slot_state;
    return state ? make_connection(*state) : connection{};
}

} // namespace this_signal
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
