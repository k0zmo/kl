#pragma once

#include "kl/detail/macros.hpp"

#include <boost/version.hpp>

#include <cstddef>

/*
 * Requirements: boost 1.57+, C++14 compiler and preprocessor
 * Sample usage:

     namespace ns {

     enum class enum_
     {
         A, B, C
     };
     KL_REFLECT_ENUM(enum_, A, (B, bb), C)
     }

 * First argument is unqualified enum type name.
 * The rest is a list of enum values. Each value can be optionally a
   tuple (pair) of its real name and name used for to-from string conversions.
 * Macro should be placed inside the same namespace as the enum type.

 * Use kl::enum_reflector<ns::enum_> to query enum properties:
     kl::enum_reflector<ns::enum_>::to_string(ns::enum_{C});
     kl::enum_reflector<ns::enum_>::from_string("bb");
     kl::enum_reflector<ns::enum_>::count();
     kl::enum_reflector<ns::enum_>::values();
 * Alternatively, use kl::reflect<ns::enum_>()

 * Remarks: Macro KL_REFLECT_ENUM works for unscoped enums as well as scoped
   enums.

 * Above definition is expanded to:

     namespace kl_reflect_enum_ {
     inline constexpr ::kl::enum_reflection_pair<enum_> reflection_data[] = {
         {enum_::A, "A"},
         {enum_::B, "bb"},
         {enum_::C, "C"}};
     }
     constexpr auto reflect_enum(::kl::enum_class<enum_>) noexcept
     {
         return ::kl::enum_reflection_view{kl_reflect_enum_::reflection_data};
     }

 * KL_REFLECT_ENUM uses Boost PP's sequences under the hood. Despite that, the
   conversion from variadics to a sequence goes through a tuple which is limited
   to 64 arguments. Therefore KL_REFLECT_ENUM is limited to 64 enum values. You
   can circumvent this issue by using alternative syntax with
   KL_REFLECT_ENUM_SEQ as such:

     namespace ns {

     enum class enum_
     {
         A, B, C
     };
     KL_REFLECT_ENUM_SEQ(enum_, (A)(B, bb)(C))
     }

   With that, the limit goes up to 256 enum values.

 * Alternatively, starting from Boost 1.75 you can increase tuple's limit by
   defining BOOST_PP_LIMIT_TUPLE to 128 or 256. However, such change has no
   effect on non-conforming preprocessor such as MSVC without /Zc:preprocessor
   flag. Additionaly, sequence's limit can also be increased by defining
   BOOST_PP_LIMIT_SEQ to 512 or 1024. Consult
   https://www.boost.org/doc/libs/master/libs/preprocessor/doc/topics/limitations.html
   for more information.

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

template <typename Enum, std::size_t N>
class enum_reflection_view
{
public:
    constexpr explicit enum_reflection_view(
        const kl::enum_reflection_pair<Enum> (&arr)[N]) noexcept
        : first_{arr}
    {
    }

    constexpr auto begin() const noexcept { return first_; }
    constexpr auto end() const noexcept { return first_ + N; }
    constexpr auto size() const noexcept { return N; }

    template <std::size_t I>
    constexpr const enum_reflection_pair<Enum>& get() const noexcept
    {
        static_assert(I < N);
        return *(first_ + I);
    }

private:
    const kl::enum_reflection_pair<Enum>* first_;
};
} // namespace kl

#define KL_REFLECT_ENUM(name_, ...)                                            \
    KL_REFLECT_ENUM_SEQ(name_, KL_VARIADIC_TO_SEQ(__VA_ARGS__))

#define KL_REFLECT_ENUM_SEQ(name_, values_)                                    \
    namespace KL_REFLECT_ENUM_NSNAME(name_) {                                  \
    inline constexpr ::kl::enum_reflection_pair<name_> reflection_data[] = {   \
        KL_REFLECT_ENUM_REFLECTION_PAIRS(name_, values_)};                     \
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
        return "unknown <" KL_STRINGIZE(name_) ">";                            \
    }

#define KL_REFLECT_ENUM_NSNAME(name_) KL_CONCAT(kl_reflect_, name_)

#define KL_REFLECT_ENUM_REFLECTION_PAIRS(name_, values_)                       \
    KL_SEQ_FOR_EACH2(name_, values_, KL_REFLECT_ENUM_REFLECTION_PAIR)

#define KL_REFLECT_ENUM_REFLECTION_PAIR(name_, value_)                         \
    KL_REFLECT_ENUM_REFLECTION_PAIR2(name_, KL_TUPLE_EXTEND_BY_FIRST(value_))

// Assumes value_ is a tuple: (x, x) or (x, y) where `x` is the name and `y` is
// the string form of the enum value
#define KL_REFLECT_ENUM_REFLECTION_PAIR2(name_, value_)                        \
    {name_::KL_TUPLE_ELEM(0, value_), KL_STRINGIZE(KL_TUPLE_ELEM(1, value_))},

#if BOOST_VERSION < 107500 \
 || BOOST_PP_LIMIT_TUPLE < 128 \
 || (defined(_MSC_VER) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL))
#  define KL_REFLECT_ENUM_LIMIT 64
#else
#  define KL_REFLECT_ENUM_LIMIT BOOST_PP_LIMIT_TUPLE
#endif
