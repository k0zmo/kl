#pragma once

#include <rapidjson/fwd.h>

#include <string>

namespace kl::json {

class view;

template <typename Writer>
class dump_context;

template <typename T>
std::string dump(const T& obj);

template <typename T, typename Context>
void dump(const T& obj, Context& ctx);

template <typename T>
struct serializer;

using default_allocator =
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

class owning_serialize_context;
class serialize_context;

template <typename T>
rapidjson::Document serialize(const T& obj);

template <typename T, typename Context>
rapidjson::Value serialize(const T& obj, Context& ctx);

template <typename T>
void deserialize(T& out, const rapidjson::Value& value);

template <typename T>
T deserialize(const rapidjson::Value& value);

struct deserialize_error;
struct parse_error;
} // namespace kl::json
