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
    constexpr range() : first_{}, last_{} {}
    constexpr range(Iterator first, Iterator last)
        : first_{std::move(first)}, last_{std::move(last)}
    {
    }

    constexpr Iterator begin() const { return first_; }
    constexpr Iterator end() const { return last_; }
    constexpr std::size_t size() const { return std::distance(first_, last_); }

private:
    Iterator first_;
    Iterator last_;
};

template <class Iterator>
inline constexpr range<Iterator> make_range(Iterator a, Iterator b)
{
    return range<Iterator>{a, b};
}

template <class Container>
inline range<typename Container::iterator> make_range(Container& cont)
{
    return range<typename Container::iterator>{cont.begin(), cont.end()};
}

template <class Container>
inline range<typename Container::const_iterator>
    make_range(const Container& cont)
{
    return range<typename Container::const_iterator>{cont.cbegin(),
                                                     cont.cend()};
}

template <class ArrayType, std::size_t N>
inline constexpr range<std::add_pointer_t<ArrayType>>
    make_range(ArrayType (&arr)[N])
{
    using value_type = std::add_pointer_t<ArrayType>;
    return range<value_type>{std::begin(arr), std::end(arr)};
}

template <class ArrayType, std::size_t N>
inline constexpr range<std::add_pointer_t<const ArrayType>>
    make_range(const ArrayType (&arr)[N])
{
    using value_type = std::add_pointer_t<const ArrayType>;
    return range<value_type>{std::begin(arr), std::end(arr)};
}
} // namespace kl
