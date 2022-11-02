#pragma once

// Based on http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0640r0.pdf

#include <boost/core/demangle.hpp>

#include <sstream>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <cstddef>

namespace kl {

namespace detail {

class exception_info_node_base
{
public:
    virtual ~exception_info_node_base() noexcept = default;

    virtual const void* find(const std::type_info& tag) const noexcept = 0;
    virtual void* find(const std::type_info& tag) noexcept = 0;
    virtual void diagnostic_info(std::ostream& os) = 0;

    std::shared_ptr<exception_info_node_base> next_{};
};

struct demangled_type_name
{
    friend std::ostream& operator<<(std::ostream& os, demangled_type_name dtn)
    {
        boost::core::scoped_demangled_name name{dtn.ti.name()};
        const char* p = name.get();
        if (!p)
            p = dtn.ti.name();
        return os << p;
    }

    const std::type_info& ti;
};

template <typename T>
auto diagnostic_info_impl(std::ostream& os, const std::type_info& tag,
                          const T& value, int)
    -> decltype((os << value), void())
{
    os << demangled_type_name{tag} << " = " << value << '\n';
}

template <typename T>
void diagnostic_info_impl(std::ostream& os, const std::type_info& tag,
                          const T& value, ...)
{
    os << demangled_type_name{tag} << " = [";

    constexpr const char* hex = "0123456789ABCDEF";
    const auto bytes =
        reinterpret_cast<const unsigned char*>(std::addressof(value));
    const auto num_bytes = sizeof(value);

    for (std::size_t i = 0; i < num_bytes; ++i)
    {
        if (i > 0)
            os << ' ';
        os << hex[(bytes[i] & 0xF0) >> 4] << hex[bytes[i] & 0xF];
    }

    os << "]\n";
}

template <typename Tag>
class exception_info_node : public exception_info_node_base
{
public:
    explicit exception_info_node(const typename Tag::type& value)
        : value_{value}
    {
    }
    explicit exception_info_node(typename Tag::type&& value)
        : value_{std::move(value)}
    {
    }

    const void* find(const std::type_info& tag) const noexcept override
    {
        return tag == typeid(Tag) ? &value_ : nullptr;
    }

    void* find(const std::type_info& tag) noexcept override
    {
        return tag == typeid(Tag) ? &value_ : nullptr;
    }

    void diagnostic_info(std::ostream& os) override
    {
        diagnostic_info_impl(os, typeid(Tag), value_, 0);
    }

private:
    typename Tag::type value_;
};
} // namespace detail

#if defined(exception_info)
#error "exception_info already defined (excpt.h file dragged by Windows.h?)"
#endif

class exception_info
{
public:
    exception_info() noexcept = default;
    exception_info(const char* file, int line, const char* function) noexcept
        : file_{file}, line_{line}, function_{function}
    {
    }

    virtual ~exception_info() noexcept = default;

    exception_info(const exception_info& other) = default;
    exception_info(exception_info&& other) noexcept = default;

    exception_info& operator=(const exception_info& other) = default;
    exception_info& operator=(exception_info&& other) noexcept = default;

    [[nodiscard]] const char* file() const noexcept { return file_; }
    [[nodiscard]] int line() const noexcept { return line_; }
    [[nodiscard]] const char* function() const noexcept { return function_; }

    template <typename Tag>
    exception_info& set(const typename Tag::type& x)
    {
        auto node = std::make_shared<detail::exception_info_node<Tag>>(x);
        node->next_ = head_;
        head_ = node;
        return *this;
    }

    template <typename Tag>
    exception_info& set(typename Tag::type&& x)
    {
        auto node =
            std::make_shared<detail::exception_info_node<Tag>>(std::move(x));
        node->next_ = head_;
        head_ = node;
        return *this;
    }

    template <typename Tag>
    exception_info& unset() noexcept
    {
        if (!head_)
            return *this;

        std::shared_ptr<detail::exception_info_node_base>* node = &head_;
        std::shared_ptr<detail::exception_info_node_base>* prev = nullptr;
        const auto& tag = typeid(Tag);

        while (*node)
        {
            if ((*node)->find(tag))
            {
                if (prev)
                    (*prev)->next_ = (*node)->next_;
                else
                    head_ = (*node)->next_;
                break;
            }

            prev = node;
            node = &(*node)->next_;
        }

        return *this;
    }

    template <typename Tag>
    [[nodiscard]] const typename Tag::type* get() const noexcept
    {
        return const_cast<exception_info*>(this)->template get<Tag>();
    }

    template <typename Tag>
    [[nodiscard]] typename Tag::type* get() noexcept
    {
        const auto& tag = typeid(Tag);
        for (auto* node = &head_; *node;)
        {
            if (void* ptr = (*node)->find(tag); ptr)
                return static_cast<typename Tag::type*>(ptr);
            node = &(*node)->next_;
        }

        return nullptr;
    }

    [[nodiscard]] std::string diagnostic_info() const
    {
        std::ostringstream ss;
        if (file_)
            ss << file_ << '(' << line_ << ')';
        else
            ss << "<unknown-file>";

        ss << ": throw_with_info in function "
           << (function_ ? function_ : "<unknown-function>") << '\n';
        for (auto* node = &head_; *node;)
        {
            (*node)->diagnostic_info(ss);
            node = &(*node)->next_;
        }
        return ss.str();
    }

private:
    const char* file_{};
    int line_{};
    const char* function_{};

    // Not unique because of copyability requirement
    std::shared_ptr<detail::exception_info_node_base> head_{};
};

namespace detail {

struct exception_with_info_base : public exception_info
{
    exception_with_info_base(const std::type_info& exception_type,
                             exception_info xi)
        : exception_info{std::move(xi)}, exception_type{exception_type}
    {
    }

    const std::type_info& exception_type;
};

template <typename E>
class exception_with_info : public E, public exception_with_info_base
{
public:
    exception_with_info(const E& e, exception_info xi)
        : E{e}, exception_with_info_base{typeid(E), std::move(xi)}
    {
    }

    exception_with_info(E&& e, exception_info xi)
        : E{std::move(e)}, exception_with_info_base{typeid(E), std::move(xi)}
    {
    }
};
} // namespace detail

template <typename E>
[[noreturn]] void throw_with_info(E&& e, exception_info&& xi = exception_info())
{
    using e_type = std::remove_cv_t<std::remove_reference_t<E>>;

    static_assert(std::is_class_v<e_type> && !std::is_final_v<e_type>);
    static_assert(!std::is_base_of_v<exception_info, e_type>);

    throw detail::exception_with_info<e_type>(std::forward<E>(e),
                                              std::move(xi));
}

template <typename E>
[[noreturn]] void throw_with_info(E&& e, const exception_info& xi)
{
    throw_with_info(std::forward<E>(e), exception_info{xi});
}

template <typename E>
[[nodiscard]] const exception_info* get_exception_info(const E& e)
{
    static_assert(std::is_polymorphic_v<E>);
    return dynamic_cast<const exception_info*>(std::addressof(e));
}

template <typename E>
[[nodiscard]] exception_info* get_exception_info(E& e)
{
    static_assert(std::is_polymorphic_v<E>);
    return dynamic_cast<exception_info*>(std::addressof(e));
}

template <typename E>
[[nodiscard]] std::string exception_diagnostic_info([[maybe_unused]] const E& e)
{
    std::ostringstream ss;
    using detail::demangled_type_name;

    if constexpr (!std::is_polymorphic_v<E>)
    {
        ss << "dynamic exception type: " << demangled_type_name{typeid(E)}
           << '\n';
    }
    else
    {
        const E* ep = std::addressof(e);
        if (const auto* p =
                dynamic_cast<const detail::exception_with_info_base*>(ep))
        {
            ss << "dynamic exception type: "
               << demangled_type_name{p->exception_type} << '\n';
        }
        else
        {
            ss << "dynamic exception type: " << demangled_type_name{typeid(e)}
               << '\n';
        }

        if (const auto* p = dynamic_cast<const std::exception*>(ep))
            ss << "what: " << p->what() << '\n';

        if (const auto* p = dynamic_cast<const exception_info*>(ep))
            ss << p->diagnostic_info();
    }

    return ss.str();
}

[[nodiscard]] inline std::string
    exception_diagnostic_info(const std::exception_ptr& p)
try
{
    if (p)
        std::rethrow_exception(p);
    return "no exception";
}
catch (const exception_info& xi)
{
    return exception_diagnostic_info(xi);
}
catch (const std::exception& ex)
{
    return exception_diagnostic_info(ex);
}
catch (...)
{
    return "dynamic exception type: <unknown-exception>\n";
}

#if defined(_MSC_VER)
#  define KL_FUNCTION_NAME __FUNCSIG__
#elif defined(__GNUG__)
#  define KL_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#  define KL_FUNCTION_NAME __func__
#endif

#define KL_MAKE_EXCEPTION_INFO()                                               \
    ::kl::exception_info{__FILE__, __LINE__, KL_FUNCTION_NAME}
} // namespace kl
