#pragma once

#include <iterator>
#include <algorithm>
#include <vector>
#include <string>

namespace kl {

// See http://tristanbrindle.com/posts/a-quicker-study-on-tokenising/
template <typename InputIterator, typename ForwardIterator, typename BinOp>
void for_each_token(InputIterator first, InputIterator last,
                    ForwardIterator s_first, ForwardIterator s_last,
                    BinOp binary_op)
{
    while (first != last)
    {
        const auto pos = std::find_first_of(first, last, s_first, s_last);
        binary_op(first, pos);
        if (pos == last)
            break;
        first = std::next(pos);
    }
}

template <typename String, typename Delimiters>
inline std::vector<std::string>
    split(const String& str, const Delimiters& delims, bool skip_empty = true)
{
    using std::cbegin;
    using std::cend;

    std::vector<std::string> output;
    for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                   [&](auto first, auto last) {
                       if (first != last || !skip_empty)
                           output.emplace_back(first, last);
                   });
    return output;
}
} // namespace kl
