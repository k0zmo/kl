#pragma once

#include "kl/tuple.hpp"

#include <iterator>
#include <tuple>
#include <type_traits>

namespace kl {
namespace detail {

template <typename Seq>
struct seq_traits
{
    using iterator = decltype(std::begin(std::declval<Seq&>()));
    using sentinel = decltype(std::end(std::declval<Seq&>()));
    using reference = decltype(*std::begin(std::declval<Seq&>()));
};

template <typename Seq, std::size_t N>
struct seq_traits<Seq[N]>
{
    using iterator = std::add_pointer_t<Seq>;
    using sentinel = iterator;
    using reference = std::add_lvalue_reference_t<Seq>;
};

template <typename Seq, std::size_t N>
struct seq_traits<const Seq[N]>
{
    using iterator = std::add_pointer_t<const Seq>;
    using sentinel = iterator;
    using reference = std::add_lvalue_reference_t<const Seq>;
};

template <typename Seq>
struct seq_traits<Seq*>
{
    static_assert(sizeof(Seq) == 0, "No Sequence traits for pointer types");
};

template <typename Seq>
struct seq_traits<const Seq*> : seq_traits<Seq*>
{
};

template <typename... Seqs>
class zipped_sentinel
{
    static_assert(sizeof...(Seqs) > 0, "Empty zip sequence not allowed");

public:
    explicit zipped_sentinel(typename seq_traits<Seqs>::sentinel... sentinels)
        : pack_(std::make_tuple(std::move(sentinels)...))
    {
    }

    template <typename Tuple>
    bool not_equal(const Tuple& tup) const
    {
        return tuple::not_equal_fn::call(tup, pack_);
    }

private:
     std::tuple<typename seq_traits<Seqs>::sentinel...> pack_;
};

template <typename... Seqs>
class zipped_iterator
{
    static_assert(sizeof...(Seqs) > 0, "Empty zip sequence not allowed");

public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::tuple<typename seq_traits<Seqs>::reference...>;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = value_type*;

public:
    explicit zipped_iterator(typename seq_traits<Seqs>::iterator... iters)
        : pack_(std::make_tuple(std::move(iters)...))
    {
    }

    reference operator*() const { return tuple::transform_ref_fn::call(pack_); }

    zipped_iterator& operator++()
    {
        tuple::for_each_fn::call(pack_, [](auto& f) { ++f; });
        return *this;
    }

    friend bool operator!=(const zipped_iterator& self,
                           const zipped_sentinel<Seqs...>& sentinel)
    {
        return sentinel.not_equal(self.pack_);
    }

private:
    std::tuple<typename seq_traits<Seqs>::iterator...> pack_;
};

class unbounded_sentinel {};

template <typename T>
class iota_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = value_type*;

public:
    explicit constexpr iota_iterator(T begin) noexcept : value_{begin} {}

    constexpr reference operator*() const noexcept { return value_; }

    constexpr iota_iterator& operator++() noexcept
    {
        ++value_;
        return *this;
    }

    constexpr bool operator!=(iota_iterator other) const noexcept
    {
        return value_ != other.value_;
    }

    constexpr bool operator!=(unbounded_sentinel) const noexcept
    {
        return true;
    }

private:
    T value_;
};
} // namespace detail

template <typename... Seqs>
class zipped_range
{
public:
    explicit zipped_range(const Seqs&... seqs)
        : first_{std::begin(seqs)...}, last_{std::end(seqs)...}
    {
    }

    auto begin() const { return first_; }
    auto end() const { return last_; }

private:
    detail::zipped_iterator<std::remove_reference_t<Seqs>...> first_;
    detail::zipped_sentinel<std::remove_reference_t<Seqs>...> last_;
};

template <typename T>
class iota_range
{
public:
    constexpr iota_range(T begin, T end) noexcept : begin_{begin}, end_{end} {}

    constexpr auto begin() const noexcept
    {
        return detail::iota_iterator<T>{begin_};
    }
    constexpr auto end() const noexcept
    {
        return detail::iota_iterator<T>{end_};
    }

private:
    T begin_;
    T end_;
};

template <typename T>
class unbounded_iota_range
{
public:
    explicit constexpr unbounded_iota_range(T begin = {}) noexcept
        : begin_{begin}
    {
    }

    constexpr auto begin() const noexcept
    {
        return detail::iota_iterator<T>{begin_};
    }
    constexpr auto end() const noexcept { return detail::unbounded_sentinel{}; }

private:
    T begin_;
};

template <typename Seq>
inline constexpr bool is_borrowed_range = false;

template <typename T>
inline constexpr bool is_borrowed_range<unbounded_iota_range<T>> = true;

template <typename T>
inline constexpr bool is_borrowed_range<iota_range<T>> = true;

template <typename... Seqs>
zipped_range<Seqs...> zipped(Seqs&&... seqs)
{
    constexpr bool is_safely_zipped =
        (... &&
         (!std::is_rvalue_reference_v<Seqs&&> || kl::is_borrowed_range<Seqs>));
    static_assert(
        is_safely_zipped,
        "kl::zipped doesn't work on prvalue ranges unless given range is "
        "defined otherwise using is_borrowed_range trait");
    return zipped_range<Seqs...>{std::forward<Seqs>(seqs)...};
}

template <typename Seq>
zipped_range<unbounded_iota_range<std::size_t>, Seq> enumerated(Seq&& seq)
{
    // Calling zipped() with prvalue unbounded_iota_range is safe since its
    // iterators are copied and can be held inside zipped_range outliving their
    // range parent because they never ever refer to it after construction
    return kl::zipped(unbounded_iota_range<std::size_t>{0U},
                      std::forward<Seq>(seq));
}

} // namespace kl
