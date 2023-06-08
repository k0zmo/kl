#pragma once

#include <concepts>
#include <string>

namespace YAML {

class Node;
class Emitter;
} // namespace YAML

namespace kl::yaml {

class view;

template <typename T>
concept dump_context = requires(T& a) {
    { a.emitter() } -> std::same_as<YAML::Emitter&>;
    { a.skip_field("abc", 0) } -> std::same_as<bool>;
};

template <typename T>
concept serialize_context = requires(T& a) {
    { a.skip_field("abc", 0) } -> std::same_as<bool>;
};

class default_dump_context;

template <typename T>
std::string dump(const T& obj);

template <typename T>
void dump(const T& obj, dump_context auto& ctx);

template <typename T>
struct serializer;

class default_serialize_context;

template <typename T>
YAML::Node serialize(const T& obj);

template <typename T>
YAML::Node serialize(const T& obj, serialize_context auto& ctx);

template <typename T>
void deserialize(T& out, const YAML::Node& value);

template <typename T>
T deserialize(const YAML::Node& value);

struct deserialize_error;
struct parse_error;

} // namespace kl::yaml
