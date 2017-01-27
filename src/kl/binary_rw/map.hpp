#pragma once

#include "kl/binary_rw.hpp"

#include <map>

namespace kl {

template <typename K, typename V>
kl::binary_writer& operator<<(kl::binary_writer& w, const std::map<K ,V>& map)
{
    w << static_cast<std::uint32_t>(map.size());

    for (const auto& kv : map)
    {
        w << kv.first;
        w << kv.second;
    }

    return w;
}

template <typename K, typename V>
kl::binary_reader& operator>>(kl::binary_reader& r, std::map<K, V>& map)
{
    const auto size = r.read<std::uint32_t>();
    map.clear();

    for (std::uint32_t i = 0; i < size; ++i)
    {
        map.insert({r.read<K>(), r.read<V>()});
        if (r.err())
        {
            map.clear();
            break;
        }
    }

    return r;
}
} // namespace kl
