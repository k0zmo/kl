#pragma once

#include <iterator>
#include <type_traits>
#include <cstddef>

namespace kl {

template <class Iterator>
class range
{
public:
    using iterator = Iterator;
    using iterator_traits = std::iterator_traits<Iterator>;
    using value_type = typename iterator_traits::value_type;
    using difference_type = typename iterator_traits::difference_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;

public:
    constexpr range() noexcept(std::is_nothrow_constructible<Iterator>::value)
        : first_{}, last_{}
    {
    }
    constexpr range(Iterator first, Iterator last) noexcept(
        std::is_nothrow_move_constructible<Iterator>::value)
        : first_{std::move(first)}, last_{std::move(last)}
    {
    }

    constexpr Iterator begin() const
        noexcept(std::is_nothrow_copy_constructible<Iterator>::value)
    {
        return first_;
    }

    constexpr Iterator end() const
        noexcept(std::is_nothrow_copy_constructible<Iterator>::value)
    {
        return last_;
    }

    constexpr std::size_t size() const
        noexcept(noexcept(std::distance(std::declval<Iterator>(),
                                        std::declval<Iterator>())))
    {
        return std::distance(first_, last_);
    }

private:
    Iterator first_;
    Iterator last_;
};

template <class Iterator>
constexpr range<Iterator> make_range(Iterator a, Iterator b) noexcept(
    noexcept(range<Iterator>{std::move(a), std::move(b)}))
{
    return range<Iterator>{std::move(a), std::move(b)};
}

template <class Container>
range<typename Container::iterator> make_range(Container& cont) noexcept(
    noexcept(range<typename Container::iterator>{cont.begin(), cont.end()}))
{
    return range<typename Container::iterator>{cont.begin(), cont.end()};
}

template <class Container>
range<typename Container::const_iterator>
    make_range(const Container& cont) noexcept(noexcept(
        range<typename Container::const_iterator>{cont.cbegin(), cont.cend()}))
{
    return range<typename Container::const_iterator>{cont.cbegin(),
                                                     cont.cend()};
}

template <class ArrayType, std::size_t N>
constexpr range<std::add_pointer_t<ArrayType>>
    make_range(ArrayType (&arr)[N]) noexcept
{
    using value_type = std::add_pointer_t<ArrayType>;
    return range<value_type>{std::begin(arr), std::end(arr)};
}

template <class ArrayType, std::size_t N>
constexpr range<std::add_pointer_t<const ArrayType>>
    make_range(const ArrayType (&arr)[N]) noexcept
{
    using value_type = std::add_pointer_t<const ArrayType>;
    return range<value_type>{std::begin(arr), std::end(arr)};
}
} // namespace kl
