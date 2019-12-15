#pragma once

#include "kl/base_range.hpp"
#include "kl/tuple.hpp"
#include "kl/type_traits.hpp"

#include <tuple>
#include <type_traits>
#include <iterator>
#include <limits>

namespace kl {
namespace detail {

template <typename Seq>
struct seq_traits
{
private:
    using seq = typename std::remove_reference<Seq>::type;

public:
    using iterator = typename seq::iterator;
    using reference = typename seq::reference;
};

template <typename Seq>
struct seq_traits<const Seq>
{
private:
    using seq = typename std::remove_reference<Seq>::type;

public:
    using iterator = typename seq::const_iterator;
    using reference = typename seq::const_reference;
};

template <typename Seq, std::size_t N>
struct seq_traits<Seq[N]>
{
    using iterator = typename std::add_pointer<Seq>::type;
    using reference = typename std::add_lvalue_reference<Seq>::type;
};

template <typename Seq, std::size_t N>
struct seq_traits<const Seq[N]>
{
    using iterator = typename std::add_pointer<const Seq>::type;
    using reference = typename std::add_lvalue_reference<const Seq>::type;
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
class zip_iterator
{
    static_assert(sizeof...(Seqs) > 0, "Empty zip sequence not allowed");

public:
    using zip_pack = std::tuple<typename seq_traits<Seqs>::iterator...>;

    using iterator_category = std::input_iterator_tag;
    using value_type = std::tuple<typename seq_traits<Seqs>::reference...>;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = value_type*;

public:
    explicit zip_iterator(zip_pack pack) : pack_(std::move(pack)) {}
    explicit zip_iterator(typename seq_traits<Seqs>::iterator... iters)
        : pack_(std::make_tuple(std::move(iters)...))
    {
    }

    zip_iterator& operator++()
    {
        tuple::for_each_fn::call(pack_, [](auto& f) { ++f; });
        return *this;
    }

    reference operator*() const
    {
        return tuple::transform_ref_fn::call(pack_);
    }

    bool operator!=(const zip_iterator& other) const
    {
        return tuple::not_equal_fn::call(pack_, other.pack_);
    }

    difference_type distance_to(const zip_iterator& other) const
    {
        return tuple::distance_fn::call(pack_, other.pack_);
    }

private:
    zip_pack pack_;
};

class integral_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = value_type*;

public:
    explicit integral_iterator(std::size_t value) : value_{value} {}

    reference operator*() const { return value_; }

    integral_iterator& operator++()
    {
        ++value_;
        return *this;
    }

    bool operator!=(const integral_iterator& other) const
    {
        return value_ != other.value_;
    }

    difference_type operator-(const integral_iterator& other) const
    {
        return value_ - other.value_;
    }

private:
    std::size_t value_{0};
};

class inf_integral_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = value_type*;

public:
    explicit inf_integral_iterator(std::size_t value) : value_{value} {}

    reference operator*() const { return value_; }

    inf_integral_iterator& operator++()
    {
        ++value_;
        return *this;
    }

    bool operator!=(const inf_integral_iterator&) const { return true; }

private:
    std::size_t value_{0};
};
} // namespace detail

template <typename... Seqs>
class zip_range
    : public base_range<
          detail::zip_iterator<typename std::remove_reference<Seqs>::type...>>
{
public:
    using iterator =
        detail::zip_iterator<typename std::remove_reference<Seqs>::type...>;
    using super_t = base_range<iterator>;

public:
    explicit zip_range(const Seqs&... seqs)
        : super_t{iterator{std::begin(seqs)...}, iterator{std::end(seqs)...}}
    {
    }

    std::size_t size() const
    {
        return super_t::begin().distance_to(super_t::end());
    }
};

class integral_range : public base_range<detail::integral_iterator>
{
public:
    using iterator = detail::integral_iterator;
    using super_t = base_range<iterator>;

public:
    integral_range(std::size_t first, std::size_t last)
        : super_t{iterator{first}, iterator{last}}
    {
    }

    size_t size() const { return end() - begin(); }
};

class inf_integral_range : public base_range<detail::inf_integral_iterator>
{
public:
    using iterator = detail::inf_integral_iterator;
    using super_t = base_range<iterator>;

public:
    inf_integral_range() : super_t{iterator{0}, iterator{0}} {}

    size_t size() const { return 0; }
};

template <typename Seq>
integral_range make_integral_range(const Seq& seq)
{
    return {0, seq.size()};
}

template <typename Seq, std::size_t N>
integral_range make_integral_range(const Seq (&seq)[N])
{
    return {0, N};
}

template <typename Seq>
struct is_rvalue_compatible : std::false_type {};
template <>
struct is_rvalue_compatible<inf_integral_range> : std::true_type {};
template <>
struct is_rvalue_compatible<integral_range> : std::true_type {};

template <typename... Seqs>
zip_range<Seqs...> make_zip(Seqs&&... seqs)
{
    static_assert(
        conjunction<disjunction<negation<std::is_rvalue_reference<Seqs&&>>,
                                is_rvalue_compatible<Seqs>>...>::value,
        "make_zip doesn't work on prvalue ranges unless given range "
        "is defined otherwise using is_rvalue_compatible trait");
    return zip_range<Seqs...>{std::forward<Seqs>(seqs)...};
}

template <typename Seq>
zip_range<inf_integral_range, Seq> make_enumeration(Seq&& seq)
{
    // Calling make_zip() with prvalue inf_integral_range is safe since its
    // iterators are copied and can be held inside zip_range outliving their
    // range parent because they never ever refer to it after construction
    return make_zip(inf_integral_range{}, std::forward<Seq>(seq));
}

template <typename ZipSeq, typename Fun>
void zip_for_each(ZipSeq&& zipSeq, Fun&& fun)
{
    for (auto&& it : std::forward<ZipSeq>(zipSeq))
    {
        tuple::apply_fn::call(std::forward<decltype(it)>(it),
                              std::forward<Fun>(fun));
    }
}

template <typename Seq, typename Fun>
void enumerate(Seq&& seq, Fun&& fun)
{
    zip_for_each(make_enumeration(std::forward<Seq>(seq)),
                 std::forward<Fun>(fun));
}
} // namespace zip
