#pragma once

#include <rapidjson/fwd.h>

#include <concepts>
#include <string>

namespace kl::json {

using allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

class view;

template <typename T>
concept dump_context = requires(T& a) {
    typename T::writer_type;
    { a.writer() } -> std::same_as<typename T::writer_type&>;
    { a.skip_field("abc", 0) } -> std::same_as<bool>;
};

template <typename T>
concept serialize_context = requires(T& a) {
    { a.allocator() } -> std::same_as<json::allocator&>;
    { a.skip_field("abc", 0) } -> std::same_as<bool>;
};

template <typename Writer>
class default_dump_context;

template <typename T>
std::string dump(const T& obj);

template <typename T>
void dump(const T& obj, dump_context auto& ctx);

template <typename T>
struct serializer;

class owning_serialize_context;
class borrowing_serialize_context;

template <typename T>
rapidjson::Document serialize(const T& obj);

template <typename T>
rapidjson::Value serialize(const T& obj, serialize_context auto& ctx);

template <typename T>
void deserialize(T& out, const rapidjson::Value& value);

template <typename T>
T deserialize(const rapidjson::Value& value);

struct deserialize_error;
struct parse_error;
} // namespace kl::json
