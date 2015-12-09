#pragma once

#include <iterator>
#include <type_traits>

namespace kl {

template <class Iterator>
class base_range
{
public:
    using iterator = Iterator;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type =
        typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;

public:
    explicit base_range() = default;
    base_range(Iterator first, Iterator last)
        : first_{std::move(first)}, last_{std::move(last)}
    {
    }

    Iterator begin() const { return first_; }
    Iterator end() const { return last_; }

    // default implementation (iterator_category is used for dispatching)
    size_t size() const { return std::distance(first_, last_); }

private:
    Iterator first_, last_;
};

template <class Iterator>
inline base_range<Iterator> make_range(Iterator a, Iterator b)
{
    return base_range<Iterator>{a, b};
}

template <class Container>
inline base_range<typename Container::iterator> make_range(Container& cont)
{
    return base_range<typename Container::iterator>{cont.begin(), cont.end()};
}

template <class Container>
inline base_range<typename Container::const_iterator>
    make_range(const Container& cont)
{
    return base_range<typename Container::const_iterator>{cont.cbegin(),
                                                          cont.cend()};
}

template <class ArrayType, size_t N>
inline base_range<std::add_pointer_t<ArrayType>>
    make_range(ArrayType (&arr)[N])
{
    using value_type = std::add_pointer_t<ArrayType>;
    auto b = std::begin(arr);
    auto e = std::end(arr);
    return base_range<value_type>{b, e};
}

template <class ArrayType, size_t N>
inline base_range<std::add_pointer_t<const ArrayType>>
    make_range(const ArrayType (&arr)[N])
{
    using value_type = std::add_pointer_t<const ArrayType>;
    auto b = std::begin(arr);
    auto e = std::end(arr);
    return base_range<value_type>{b, e};
}
} // namespace kl
