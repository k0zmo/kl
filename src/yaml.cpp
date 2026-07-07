#include "kl/yaml.hpp"
#include "kl/serialization_error.hpp"
#include "kl/enum_reflector.hpp"
#include "kl/reflect_enum.hpp"

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/type.h>

#include <string>

namespace YAML::NodeType {

KL_REFLECT_ENUM(value, Undefined, Null, Scalar, Sequence, Map)
} // namespace YAML

namespace kl::yaml {

namespace detail {

std::string type_name(const YAML::Node& value)
{
    return kl::to_string(value.Type());
}

void expect_scalar(const YAML::Node& value)
{
    if (value.IsScalar())
        return;

    throw serialization::deserialize_error{"type must be a scalar but is a " +
                                           type_name(value)};
}

void expect_sequence(const YAML::Node& value)
{
    if (value.IsSequence())
        return;

    throw serialization::deserialize_error{"type must be a sequence but is a " +
                                           type_name(value)};
}

void expect_map(const YAML::Node& value)
{
    if (value.IsMap())
        return;

    throw serialization::deserialize_error{"type must be a map but is a " +
                                           type_name(value)};
}
} // namespace detail
} // namespace kl::yaml
