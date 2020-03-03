#include "kl/base64.hpp"
#include "kl/utility.hpp"

namespace kl {

std::string base64_encode(gsl::span<const std::byte> s)
{
    static constexpr const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    auto lookup = [](std::byte c) {
        return alphabet[underlying_cast(c) & 0x3F];
    };

    // How many full (without padding with '=') quadruples we're gonna have
    const auto full_quad = s.size() / 3;
    const bool is_padded = s.size() > full_quad * 3;
    std::string ret(4 * (full_quad + is_padded), '\0');

    auto src = s.data();
    auto dst = ret.begin();

    for (std::ptrdiff_t i = 0; i < full_quad; ++i)
    {
        *dst++ = lookup(src[0] >> 2);
        *dst++ = lookup(((src[0] & std::byte{0x3}) << 4) | (src[1] >> 4));
        *dst++ = lookup(((src[1]) << 2) | (src[2] >> 6));
        *dst++ = lookup(((src[2])));

        src += 3;
    }

    if (is_padded)
    {
        *dst++ = lookup(src[0] >> 2);

        if (s.size() == full_quad * 3 + 2)
        {
            *dst++ = lookup(((src[0] & std::byte{0x3}) << 4) | (src[1] >> 4));
            *dst++ = lookup((src[1] << 2));
        }
        else
        {
            *dst++ = lookup((src[0] & std::byte{0x3}) << 4);
            *dst++ = '=';
        }

        *dst++ = '=';
    }

    return ret;
}

namespace {

using base64_lut = std::byte[256];

struct table_initializer
{
    table_initializer(base64_lut& table)
    {
        std::memset(table, 0x80, sizeof(table));

        for (size_t i = 'A'; i <= 'Z'; ++i)
            table[i] = static_cast<std::byte>(0 + (i - 'A'));
        for (size_t i = 'a'; i <= 'z'; ++i)
            table[i] = static_cast<std::byte>(26 + (i - 'a'));
        for (size_t i = '0'; i <= '9'; ++i)
            table[i] = static_cast<std::byte>(52 + (i - '0'));

        table[static_cast<size_t>('+')] = std::byte{62};
        table[static_cast<size_t>('/')] = std::byte{63};
    }
};

bool is_base64(std::byte b)
{
    return underlying_cast(b) <= 0x3F;
}
} // namespace

std::optional<std::vector<std::byte>> base64_decode(gsl::cstring_span<> str)
{
    // Invert lookup table used for encoding
    static base64_lut table = {};
    static table_initializer _{table};
    auto lookup = [](char index) { return table[static_cast<size_t>(index)]; };

    std::optional<std::vector<std::byte>> ret;

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

    ret = std::vector<std::byte>((str.length() * 3 / 4) - num_eqs);

    auto src = str.begin();
    auto dst = ret->begin();

    for (std::ptrdiff_t i = 0; i < full_quad; ++i)
    {
        const std::byte lut4[] = {lookup(src[0]), lookup(src[1]),
                                  lookup(src[2]), lookup(src[3])};

        // Validate range of input characters (0-63)
        if (!is_base64(lut4[0]) ||
            !is_base64(lut4[1]) ||
            !is_base64(lut4[2]) ||
            !is_base64(lut4[3]))
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = (lut4[0] << 2) | (lut4[1] >> 4);
        *dst++ = (lut4[1] << 4) | (lut4[2] >> 2);
        *dst++ = (lut4[2] << 6) | (lut4[3]);

        src += 4;
    }

    if (num_eqs == 2)
    {
        if (!is_base64(lookup(src[0])) ||
            !is_base64(lookup(src[1])))
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = (lookup(src[0]) << 2) | (lookup(src[1]) >> 4);
    }
    else if (num_eqs == 1)
    {
        if (!is_base64(lookup(src[0])) ||
            !is_base64(lookup(src[1])) ||
            !is_base64(lookup(src[2])))
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = (lookup(src[0]) << 2) | (lookup(src[1]) >> 4);
        *dst++ = (lookup(src[1]) << 4) | (lookup(src[2]) >> 2);
    }

    return ret;
}
} // namespace kl
