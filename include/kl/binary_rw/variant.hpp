#pragma once

#include "kl/binary_rw.hpp"
#include "kl/meta.hpp"

#include <variant>

namespace kl {
namespace detail {

template <typename Variant>
void visit_by_index(kl::binary_reader& r, kl::type_pack<>, Variant&,
                    std::uint8_t)
{
    r.notify_error();
}

template <typename Head, typename... Tail, typename Variant>
void visit_by_index(kl::binary_reader& r, kl::type_pack<Head, Tail...>,
                    Variant& variant, std::uint8_t index)
{
    if (index == 0)
    {
        Head value = r.read<Head>();
        if (!r.err())
            variant = std::move(value);
    }
    else
    {
        visit_by_index(r, kl::type_pack<Tail...>{}, variant, index - 1);
    }
}
} // namespace detail

template <typename... Args>
void write_binary(kl::binary_writer& w, const std::variant<Args...>& var)
{
    w << static_cast<std::uint8_t>(var.index());
    std::visit([&w](const auto& value) { w << value; }, var);
}

template <typename... Args>
void read_binary(kl::binary_reader& r, std::variant<Args...>& var)
{
    const auto index = r.read<std::uint8_t>();
    if (!r.err())
    {
        visit_by_index(r, kl::type_pack<Args...>{}, var, index);
    }
}
} // namespace kl
