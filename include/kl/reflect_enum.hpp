#pragma once

#include "kl/detail/standalone_macros.hpp"

#include <cstddef>

/*
 * Requirements: C++14 compiler and preprocessor
 * Sample usage:

namespace ns {

enum class enum_
{
    A, B, C
};
KL_REFLECT_ENUM(enum_, A, (B, bb), C)
}

 * First argument is unqualified enum type name
 * The rest is a list of enum values. Each value can be optionally a
   tuple (pair) of its real name and name used for to-from string conversions
 * Macro should be placed inside the same namespace as the enum type

 * Use kl::enum_reflector<ns::enum_> to query enum properties:
     kl::enum_reflector<ns::enum_>::to_string(ns::enum_{C});
     kl::enum_reflector<ns::enum_>::from_string("bb");
     kl::enum_reflector<ns::enum_>::count();
     kl::enum_reflector<ns::enum_>::values();
 * Alternatively, use kl::reflect<ns::enum_>()

 * Remarks: Macro KL_REFLECT_ENUM works for unscoped as well as scoped
   enums

 * Above definition is expanded to:

     namespace kl_reflect_enum_ {
     using enum_type = enum_;
     inline constexpr ::kl::enum_reflection_pair<enum_> reflection_data[] = {
         {enum_type::A, "A"},
         {enum_type::B, "bb"},
         {enum_type::C, "C"},
         {enum_type{}, nullptr}};
     }
     constexpr auto reflect_enum(::kl::enum_class<enum_>) noexcept
     {
         return ::kl::enum_reflection_view{kl_reflect_enum_::reflection_data};
     }
 */

namespace kl {

// clang-format off
template <typename Enum>
class enum_class {};

template <typename Enum>
inline constexpr auto enum_ = enum_class<Enum>{};
// clang-format on

template <typename Enum>
struct enum_reflection_pair
{
    Enum value;
    const char* name;
};

struct enum_reflection_sentinel
{
    template <typename Enum>
    constexpr friend bool operator!=(const enum_reflection_pair<Enum>* it,
                                     enum_reflection_sentinel) noexcept
    {
        return it->name;
    }
};

template <typename Enum, std::size_t N>
class enum_reflection_view
{
public:
    constexpr explicit enum_reflection_view(
        const kl::enum_reflection_pair<Enum> (&arr)[N]) noexcept
        : first_{arr}
    {
        static_assert(N > 1); // N includes null terminator
    }

    constexpr auto begin() const noexcept { return first_; }
    constexpr auto end() const noexcept { return enum_reflection_sentinel{}; }
    constexpr auto size() const noexcept { return N - 1; }

private:
    const kl::enum_reflection_pair<Enum>* first_;
};
} // namespace kl

#define KL_REFLECT_ENUM(name_, ...)                                            \
    namespace KL_REFLECT_ENUM_NSNAME(name_)                                    \
    {                                                                          \
        using enum_type = name_;                                               \
        inline constexpr ::kl::enum_reflection_pair<name_> reflection_data[] = \
            {KL_REFLECT_ENUM_REFLECTION_PAIRS(__VA_ARGS__),                    \
             {enum_type{}, nullptr}};                                          \
    }                                                                          \
    [[maybe_unused]] constexpr auto reflect_enum(                              \
        ::kl::enum_class<name_>) noexcept                                      \
    {                                                                          \
        return ::kl::enum_reflection_view{                                     \
            KL_REFLECT_ENUM_NSNAME(name_)::reflection_data};                   \
    }                                                                          \
    [[maybe_unused]] constexpr auto reflect_enum_unknown_name(                 \
        ::kl::enum_class<name_>) noexcept                                      \
    {                                                                          \
        return "unknown <" #name_ ">";                                         \
    }

#define KL_MAKE_ENUM_REFLECTION_PAIR2(value, str)                              \
    {                                                                          \
        enum_type::value, #str                                                 \
    }
#define KL_MAKE_ENUM_REFLECTION_PAIR1(pair)                                    \
    KL_MSVC_EXP(KL_MAKE_ENUM_REFLECTION_PAIR2 pair)
#define KL_REFLECT_ENUM_REFLECTION_PAIRS(...)                                  \
    KL_FOR_EACH(KL_MAKE_ENUM_REFLECTION_PAIR1, KL_MAKE_PAIRS(__VA_ARGS__))

#define KL_REFLECT_ENUM_NSNAME(name_) kl_reflect_##name_