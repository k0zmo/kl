#include "kl/base64.hpp"

namespace kl {

std::string base64_encode(gsl::span<const byte> s)
{
    static constexpr const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    auto lookup = [](int c) { return alphabet[c & 0x3F]; };

    // How many full (without padding with '=') quadruples we're gonna have
    const auto full_quad = s.size() / 3;
    const bool is_padded = s.size() > full_quad * 3;
    std::string ret(4 * (full_quad + is_padded), '\0');

    auto src = s.data();
    auto dst = ret.begin();

    for (std::ptrdiff_t i = 0; i < full_quad; ++i)
    {
        *dst++ = lookup(src[0] >> 2);
        *dst++ = lookup(((src[0] & 0x3) << 4) | (src[1] >> 4));
        *dst++ = lookup(((src[1]) << 2) | (src[2] >> 6));
        *dst++ = lookup(((src[2])));

        src += 3;
    }

    if (is_padded)
    {
        *dst++ = lookup(src[0] >> 2);

        if (s.size() == full_quad * 3 + 2)
        {
            *dst++ = lookup(((src[0] & 0x3) << 4) | (src[1] >> 4));
            *dst++ = lookup((src[1] << 2));
        }
        else
        {
            *dst++ = lookup((src[0] & 0x3) << 4);
            *dst++ = '=';
        }

        *dst++ = '=';
    }

    return ret;
}

namespace detail {

using base64_lut = byte[256];

struct table_initializer
{
    table_initializer(base64_lut& table)
    {
        std::memset(table, 0x80, sizeof(table));

        for (size_t i = 'A'; i <= 'Z'; ++i)
            table[i] = static_cast<byte>(0 + (i - 'A'));
        for (size_t i = 'a'; i <= 'z'; ++i)
            table[i] = static_cast<byte>(26 + (i - 'a'));
        for (size_t i = '0'; i <= '9'; ++i)
            table[i] = static_cast<byte>(52 + (i - '0'));

        table[static_cast<size_t>('+')] = 62;
        table[static_cast<size_t>('/')] = 63;
    }
};

bool is_base64(byte b) { return b <= 0x3F; }
} // namespace detail

boost::optional<std::vector<byte>> base64_decode(gsl::cstring_span<> str)
{
    // Invert lookup table used for encoding
    static detail::base64_lut table = {};
    static detail::table_initializer _{table};
    auto lookup = [](char index) { return table[static_cast<size_t>(index)]; };

    boost::optional<std::vector<byte>> ret;

    if ((str.length() / 4) * 4 != str.length())
        return ret;

    // Count how many consecutive '=' there are starting from the end
    size_t num_eqs = 0;
    for (auto rit = str.rbegin(); rit != str.rend() && *rit == '=';
         ++rit, ++num_eqs)
        ;
    if (num_eqs > 2)
        return ret;

    // How many full quadruples we have
    const auto full_quad = (str.length() / 4) - (!!num_eqs);

    ret = std::vector<uint8_t>((str.length() * 3 / 4) - num_eqs, 0);

    auto src = str.begin();
    auto dst = ret->begin();

    for (std::ptrdiff_t i = 0; i < full_quad; ++i)
    {
        const byte lut4[] =
        {
            lookup(src[0]),
            lookup(src[1]),
            lookup(src[2]),
            lookup(src[3])
        };

        // Validate range of input characters (0-63)
        if (!detail::is_base64(lut4[0]) ||
            !detail::is_base64(lut4[1]) ||
            !detail::is_base64(lut4[2]) ||
            !detail::is_base64(lut4[3]))
        {
            ret = boost::none;
            return ret;
        }

        *dst++ = (lut4[0] << 2) | (lut4[1] >> 4);
        *dst++ = (lut4[1] << 4) | (lut4[2] >> 2);
        *dst++ = (lut4[2] << 6) | (lut4[3]);

        src += 4;
    }

    if (num_eqs == 2)
    {
        if (!detail::is_base64(lookup(src[0])) ||
            !detail::is_base64(lookup(src[1])))
        {
            ret = boost::none;
            return ret;
        }

        *dst++ = (lookup(src[0]) << 2) | (lookup(src[1]) >> 4);
    }
    else if (num_eqs == 1)
    {
        if (!detail::is_base64(lookup(src[0])) ||
            !detail::is_base64(lookup(src[1])) ||
            !detail::is_base64(lookup(src[2])))
        {
            ret = boost::none;
            return ret;
        }

        *dst++ = (lookup(src[0]) << 2) | (lookup(src[1]) >> 4);
        *dst++ = (lookup(src[1]) << 4) | (lookup(src[2]) >> 2);
    }

    return ret;
}
} // namespace kl
