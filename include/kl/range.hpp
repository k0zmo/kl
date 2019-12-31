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

    template <typename Container>
    constexpr range(Container& cont) noexcept(
        std::is_nothrow_move_constructible<Iterator>::value && noexcept(
            cont.begin()))
        : first_{cont.begin()}, last_{cont.end()}
    {
    }

    template <typename Container>
    constexpr range(const Container& cont) noexcept(
        std::is_nothrow_move_constructible<Iterator>::value && noexcept(
            cont.cbegin()))
        : first_{cont.cbegin()}, last_{cont.cend()}
    {
    }

    template <class ArrayType, std::size_t N>
    constexpr range(ArrayType (&arr)[N]) noexcept
        : first_{std::begin(arr)}, last_{std::end(arr)}
    {
    }

    template <class ArrayType, std::size_t N>
    constexpr range(const ArrayType (&arr)[N]) noexcept
        : first_{std::begin(arr)}, last_{std::end(arr)}
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

template <typename Container>
range(Container& cont) -> range<typename Container::iterator>;

template <typename Container>
range(const Container& cont) -> range<typename Container::const_iterator>;

template <typename ArrayType, std::size_t N>
range(ArrayType (&arr)[N]) -> range<std::add_pointer_t<ArrayType>>;

template <typename ArrayType, std::size_t N>
range(const ArrayType (&arr)[N]) -> range<std::add_pointer_t<const ArrayType>>;
} // namespace kl
