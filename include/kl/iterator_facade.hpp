#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace kl {

namespace detail {

// From https://quuxplusone.github.io/blog/2019/02/06/arrow-proxy/
template <typename T>
struct arrow_proxy
{
    arrow_proxy(T&& value) : value(std::move(value)) {}
    T* operator->() noexcept { return &value; }
    T value;
};

} // namespace detail

template <typename Derived, typename Reference, typename IteratorCategory>
class iterator_facade
{
public:
    using iterator_category = IteratorCategory;
    using reference = Reference;

    using difference_type = std::ptrdiff_t;
    using value_type =
        std::conditional_t<std::is_lvalue_reference_v<reference>,
                           std::remove_reference_t<reference>, reference>;
    using pointer = std::conditional_t<
        std::is_lvalue_reference_v<reference>,
        std::add_pointer_t<std::remove_reference_t<reference>>,
        kl::detail::arrow_proxy<value_type>>;

public:
    iterator_facade() = default;

    reference operator*() const
    {
         return impl().dereference();
    }

    pointer operator->() const
    {
        if constexpr (std::is_lvalue_reference_v<reference>)
            return &impl().dereference();
        else
            return kl::detail::arrow_proxy<value_type>{impl().dereference()};
    }

    reference operator[](difference_type n) const
    {
        static_assert(std::is_convertible_v<iterator_category,
                                            std::random_access_iterator_tag>);
        auto copy = *this + n;
        return *copy;
    }

    Derived& operator++()
    {
        impl().increment();
        return impl();
    }

    Derived operator++(int)
    {
        Derived copy{impl()};
        impl().increment();
        return copy;
    }

    Derived& operator--()
    {
        static_assert(std::is_convertible_v<iterator_category,
                                            std::bidirectional_iterator_tag>);
        impl().decrement();
        return impl();
    }

    Derived operator--(int)
    {
        static_assert(std::is_convertible_v<iterator_category,
                                            std::bidirectional_iterator_tag>);
        Derived copy{impl()};
        impl().decrement();
        return copy;
    }

    Derived& operator+=(difference_type n)
    {
        static_assert(std::is_convertible_v<iterator_category,
                                            std::random_access_iterator_tag>);
        impl().advance(n);
        return impl();
    }

    Derived& operator-=(difference_type n)
    {
        static_assert(std::is_convertible_v<iterator_category,
                                            std::random_access_iterator_tag>);
        impl().advance(-n);
        return impl();
    }

    friend Derived operator+(const iterator_facade& left, difference_type n)
    {
        auto copy = left.impl();
        return copy += n;
    }

    friend Derived operator+(difference_type n, const iterator_facade& right)
    {
        return right + n;
    }

    friend Derived operator-(const iterator_facade& left, difference_type n)
    {
        auto copy = left.impl();
        return copy -= n;
    }

    friend difference_type operator-(const iterator_facade& left,
                                     const iterator_facade& right)
    {
        static_assert(std::is_convertible_v<iterator_category,
                                            std::random_access_iterator_tag>);
        return left.impl().distance_to(right.impl());
    }

    friend bool operator==(const iterator_facade& left,
                           const iterator_facade& right)
    {
        return left.impl().equal_to(right.impl());
    }

    friend bool operator!=(const iterator_facade& left,
                           const iterator_facade& right)
    {
        return !(left == right);
    }

    friend bool operator<(const iterator_facade& left,
                          const iterator_facade& right)
    {
        return right - left > 0;
    }

    friend bool operator>(const iterator_facade& left,
                          const iterator_facade& right)
    {
        return left - right > 0;
    }

    friend bool operator<=(const iterator_facade& left,
                           const iterator_facade& right)
    {
        return !(left > right);
    }

    friend bool operator>=(const iterator_facade& left,
                           const iterator_facade& right)
    {
        return !(left < right);
    }

private:
    Derived& impl() noexcept
    {
        return static_cast<Derived&>(*this);
    }

    const Derived& impl() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }
};
} // namespace kl
