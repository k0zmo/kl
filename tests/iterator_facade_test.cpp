#include "kl/iterator_facade.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace {

template <typename T, typename U>
class vec_iterator : public kl::iterator_facade<vec_iterator<T, U>, U,
                                                std::random_access_iterator_tag>
{
public:
    using super_type =
        kl::iterator_facade<vec_iterator, U, std::random_access_iterator_tag>;

    using iterator_category = typename super_type::iterator_category;
    using reference = typename super_type::reference;
    using difference_type = typename super_type::difference_type;
    using value_type = typename super_type::value_type;
    using pointer = typename super_type::pointer;

    explicit vec_iterator(const std::vector<T>& vec) noexcept : vec_{vec}, index_{0}
    {
    }

public:
    reference dereference() const { return vec_[index_]; }
    void increment() noexcept { ++index_; }
    void decrement() noexcept { --index_; }

    bool equal_to(const vec_iterator& other) const noexcept
    {
        return other.index_ == index_;
    }

    void advance(difference_type n) noexcept { index_ += n; }
    difference_type distance_to(const vec_iterator& other) const noexcept
    {
        return index_ - other.index_;
    }

private:
    const std::vector<T>& vec_;
    std::size_t index_;
};

} // namespace

using test_type = std::pair<int, bool>;

TEMPLATE_TEST_CASE("iterator facade for vector", "",
                   (vec_iterator<test_type, test_type>),
                   (vec_iterator<test_type, const test_type>),
                   (vec_iterator<test_type, const test_type&>))
{
    std::vector<test_type> vec;
    vec.push_back({1, true});
    vec.push_back({2, false});
    vec.push_back({3, true});

    TestType it{vec};
    auto it2 = it;
    CHECK((*it).first == 1);
    CHECK(it->first == 1);
    CHECK((*it).second);
    CHECK(it->second);

    ++it;
    CHECK(it->first == 2);
    CHECK_FALSE(it->second);

    CHECK_FALSE(it2 == it);
    CHECK(it2 != it);
    CHECK(it2 < it);
    CHECK(it2 <= it);
    CHECK_FALSE(it2 > it);
    CHECK_FALSE(it2 >= it);

    --it;
    CHECK(it->first == 1);
    CHECK(it2 == it);
    CHECK_FALSE(it2 != it);
    CHECK_FALSE(it2 < it);
    CHECK(it2 <= it);
    CHECK_FALSE(it2 > it);
    CHECK(it2 >= it);

    auto it3 = it + 2;
    CHECK(it3->first == 3);
    CHECK(it3 > it);

    auto it4 = 1 + it;
    CHECK(it4->first == 2);
    CHECK(it4 > it);

    auto it5 = it3 - 2;
    CHECK(it5 == it);

    auto it6 = it4--;
    CHECK(it6 == it + 1);

    CHECK(it6[0].first == 2);
    CHECK_FALSE(it6[0].second);
    CHECK(it6[1].first == 3);
    CHECK(it6[1].second);

    auto it7 = it6;
    it7++;
    CHECK(it6 == it7 - 1);
    it7--;
    CHECK(it7 == it6);
}
