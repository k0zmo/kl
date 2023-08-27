#include "kl/json.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/reflect_enum.hpp"

#include <iterator>

namespace rapidjson {

KL_REFLECT_ENUM(Type, kNullType, kFalseType, kTrueType, kObjectType, kArrayType,
                kStringType, kNumberType)
} // namespace rapidjson

namespace kl::json {

namespace detail {

std::string type_name(const rapidjson::Value& value)
{
    return kl::to_string(value.GetType());
}
} // namespace detail

void deserialize_error::add(const char* message)
{
    messages_.insert(end(messages_), '\n');
    messages_.append(message);
}

deserialize_error::~deserialize_error() noexcept = default;
parse_error::~parse_error() noexcept = default;

void expect_integral(const rapidjson::Value& value)
{
    if (value.IsInt() || value.IsInt64() || value.IsUint() || value.IsUint64())
        return;

    throw deserialize_error{"type must be an integral but is a " +
                            detail::type_name(value)};
}

void expect_number(const rapidjson::Value& value)
{
    if (value.IsNumber())
        return;

    throw deserialize_error{"type must be a number but is a " +
                            detail::type_name(value)};
}

void expect_boolean(const rapidjson::Value& value)
{
    if (value.IsBool())
        return;

    throw deserialize_error{"type must be a boolean but is a " +
                            detail::type_name(value)};
}

void expect_string(const rapidjson::Value& value)
{
    if (value.IsString())
        return;

    throw deserialize_error{"type must be a string but is a " +
                            detail::type_name(value)};
}

void expect_object(const rapidjson::Value& value)
{
    if (value.IsObject())
        return;

    throw deserialize_error{"type must be an object but is a " +
                            detail::type_name(value)};
}

void expect_array(const rapidjson::Value& value)
{
    if (value.IsArray())
        return;

    throw deserialize_error{"type must be an array but is a " +
                            detail::type_name(value)};
}
} // namespace kl::json
