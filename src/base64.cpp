#include "kl/base64.hpp"
#include "kl/utility.hpp"

#include <gsl/span>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

constexpr std::uint32_t bad_base64_char = 1U << 24;

template <bool IsUrlVariant>
constexpr std::uint32_t base64_decode_value(std::size_t c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return 26U + c - 'a';
    if (c >= '0' && c <= '9')
        return 52U + c - '0';
    if (c == (IsUrlVariant ? '-' : '+'))
        return 62U;
    if (c == (IsUrlVariant ? '_' : '/'))
        return 63U;
    return bad_base64_char;
}

template <bool IsUrlVariant, unsigned Shift>
constexpr auto make_base64_decode_table()
{
    std::array<std::uint32_t, 256> table{};
    for (std::size_t i = 0; i < table.size(); ++i)
    {
        const auto value = base64_decode_value<IsUrlVariant>(i);
        table[i] = value == bad_base64_char ? value : value << Shift;
    }
    return table;
}

template <bool IsUrlVariant, unsigned Shift>
constexpr auto base64_decode_table = make_base64_decode_table<IsUrlVariant, Shift>();

template <bool IsUrlVariant, unsigned Shift = 0>
constexpr std::uint32_t base64_decode_lookup(char c)
{
    return base64_decode_table<IsUrlVariant, Shift>[static_cast<unsigned char>(c)];
}

constexpr bool is_base64(std::uint32_t value)
{
    return value <= 0x3F;
}

template <bool IsUrlVariant>
std::optional<std::vector<std::byte>> base64_decode_impl(std::string_view str)
{
    auto lookup = [](char c) { return base64_decode_lookup<IsUrlVariant>(c); };

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
        const auto decoded = base64_decode_lookup<IsUrlVariant, 18>(src[0]) |
                             base64_decode_lookup<IsUrlVariant, 12>(src[1]) |
                             base64_decode_lookup<IsUrlVariant,  6>(src[2]) |
                             base64_decode_lookup<IsUrlVariant,  0>(src[3]);

        if (decoded >= bad_base64_char)
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = static_cast<std::byte>(decoded >> 16);
        *dst++ = static_cast<std::byte>(decoded >> 8);
        *dst++ = static_cast<std::byte>(decoded);

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

        *dst++ = static_cast<std::byte>((lookup(src[0]) << 2) |
                                        (lookup(src[1]) >> 4));
        *dst++ = static_cast<std::byte>((lookup(src[1]) << 4) |
                                        (lookup(src[2]) >> 2));
    }
    else if (tail_size == 2)
    {
        if (!is_base64(lookup(src[0])) || !is_base64(lookup(src[1])))
        {
            ret = std::nullopt;
            return ret;
        }

        *dst++ = static_cast<std::byte>((lookup(src[0]) << 2) |
                                        (lookup(src[1]) >> 4));
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
