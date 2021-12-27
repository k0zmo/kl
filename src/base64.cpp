#include "kl/base64.hpp"
#include "kl/utility.hpp"

#include <cstring>

namespace kl {

namespace {

constexpr const char base64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr const char base64url_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

template <bool IsUrlVariant>
constexpr auto base64_lookup(std::byte c)
{
    const auto i = underlying_cast(c) & 0x3F;
    if constexpr (!IsUrlVariant)
        return base64_alphabet[i];
    else
        return base64url_alphabet[i];
}

template <bool IsUrlVariant>
std::string base64_encode_impl(gsl::span<const std::byte> s)
{
    auto lookup = [](std::byte c) { return base64_lookup<IsUrlVariant>(c); };

    // How many full (without padding with '=') quadruples we're gonna have
    const auto full_quad = s.size() / 3;
    // How many input characters are left for tail
    const auto tail_size = s.size() - (full_quad * 3);

    const auto out_tail_size = [&]() -> std::size_t {
        if (!tail_size)
            return 0U;
        if constexpr (!IsUrlVariant)
            return 4U;
        else
            return tail_size + 1U;
    }();
    std::string ret(4 * full_quad + out_tail_size, '\0');

    auto src = s.data();
    auto dst = ret.begin();

    for (std::size_t i = 0U; i < full_quad; ++i)
    {
        *dst++ = lookup(src[0] >> 2);
        *dst++ = lookup(((src[0] & std::byte{0x3}) << 4) | (src[1] >> 4));
        *dst++ = lookup(((src[1]) << 2) | (src[2] >> 6));
        *dst++ = lookup(((src[2])));

        src += 3;
    }

    if (tail_size > 0)
    {
        *dst++ = lookup(src[0] >> 2);

        if (tail_size == 2)
        {
            *dst++ = lookup(((src[0] & std::byte{0x3}) << 4) | (src[1] >> 4));
            *dst++ = lookup((src[1] << 2));
        }
        else
        {

            // tail_size == 1
            *dst++ = lookup((src[0] & std::byte{0x3}) << 4);
            if constexpr (!IsUrlVariant)
                *dst++ = '=';
        }

        if constexpr (!IsUrlVariant)
            *dst++ = '=';
    }

    return ret;
}

using base64_lut = std::byte[256];

template <bool IsUrlVariant>
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

        if constexpr (!IsUrlVariant)
        {
            table[static_cast<size_t>('+')] = std::byte{62};
            table[static_cast<size_t>('/')] = std::byte{63};
        }
        else
        {
            table[static_cast<size_t>('-')] = std::byte{62};
            table[static_cast<size_t>('_')] = std::byte{63};
        }
    }
};

constexpr bool is_base64(std::byte b)
{
    return underlying_cast(b) <= 0x3F;
}

template <bool IsUrlVariant>
std::optional<std::vector<std::byte>> base64_decode_impl(std::string_view str)
{
    // Invert lookup table used for encoding
    static base64_lut table = {};
    static table_initializer<IsUrlVariant> _{table};
    auto lookup = [](char index) { return table[static_cast<size_t>(index)]; };

    std::optional<std::vector<std::byte>> ret;

    if constexpr (!IsUrlVariant)
    {
        // We're more strict in the vanilla case:
        // SGVsbG8== is rejected as the 2nd '=' is erroneous
        if ((str.length() / 4) * 4 != str.length())
            return ret;
    }

    // Count how many consecutive '=' there are starting from the end
    size_t num_eqs = 0;
    for (auto rit = str.rbegin(); rit != str.rend() && *rit == '=';
         ++rit, ++num_eqs)
        ;
    if (num_eqs > 2)
        return ret;
    if (num_eqs > 0)
        str = str.substr(0, str.length() - num_eqs);

    // How many full quadruples we have
    const auto full_quad = str.length() / 4;
    const auto tail_size = str.length() - (full_quad * 4);
    if (tail_size == 1)
    {
        // tail_size can only be 0, 2 or 3 characters
        return ret;
    }

    // For: tail_size == 2 we have 1 additional output character
    //      tail_size == 3 we have 2 additional output characters
    const auto ret_size =
        (full_quad * 3) + (tail_size == 0 ? 0 : (tail_size - 1));
    ret = std::vector<std::byte>(ret_size);

    auto src = str.begin();
    auto dst = ret->begin();

    for (std::size_t i = 0U; i < full_quad; ++i)
    {
        const std::byte lut4[] = {lookup(src[0]), lookup(src[1]),
                                  lookup(src[2]), lookup(src[3])};

        // Validate range of input characters (0-63)
        if (!is_base64(lut4[0]) || !is_base64(lut4[1]) || !is_base64(lut4[2]) ||
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

    if (tail_size == 3)
    {
        if (!is_base64(lookup(src[0])) || !is_base64(lookup(src[1])) ||
            !is_base64(lookup(src[2])))
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = (lookup(src[0]) << 2) | (lookup(src[1]) >> 4);
        *dst++ = (lookup(src[1]) << 4) | (lookup(src[2]) >> 2);
    }
    else if (tail_size == 2)
    {
        if (!is_base64(lookup(src[0])) || !is_base64(lookup(src[1])))
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = (lookup(src[0]) << 2) | (lookup(src[1]) >> 4);
    }

    return ret;
}

} // namespace

std::string base64_encode(gsl::span<const std::byte> s)
{
    return base64_encode_impl<false>(s);
}

std::string base64url_encode(gsl::span<const std::byte> s)
{
    return base64_encode_impl<true>(s);
}

std::optional<std::vector<std::byte>> base64_decode(std::string_view str)
{
    return base64_decode_impl<false>(str);
}

std::optional<std::vector<std::byte>> base64url_decode(std::string_view str)
{
    return base64_decode_impl<true>(str);
}

} // namespace kl
