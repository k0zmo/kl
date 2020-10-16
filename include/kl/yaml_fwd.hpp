#pragma once

#include <string>

namespace YAML {

class Node;
}

namespace kl::yaml {

class view;

class dump_context;

template <typename T>
std::string dump(const T& obj);

template <typename T, typename Context>
void dump(const T& obj, Context& ctx);

template <typename T>
struct serializer;

class serialize_context;

template <typename T>
YAML::Node serialize(const T& obj);

template <typename T, typename Context>
YAML::Node serialize(const T& obj, Context& ctx);

template <typename T>
void deserialize(T& out, const YAML::Node& value);

template <typename T>
T deserialize(const YAML::Node& value);

struct deserialize_error;
struct parse_error;

} // namespace kl::yaml
