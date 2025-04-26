#pragma once

#include <cstdlib>
#include <msgpack.hpp>
#include <string_view>

// For control over the dialect of msgpack.
// Override if guile extension types collide with other custom type.
#ifndef GUILE_PACK_EXT_ID_START
#define GUILE_PACK_EXT_ID_START 100
#endif // !GUILE_PACK_EXT_ID_START

namespace guile_shared {
namespace detail {

struct malloc_deleter {
  void operator()(char *v) noexcept { ::free(v); }
};

} // namespace detail

[[noreturn]] inline void panic() { std::exit(1); }

inline std::string_view display(msgpack::type::object_type typ) {
  switch (typ) {
  case msgpack::type::object_type::NIL:
    return "NIL";
  case msgpack::type::object_type::BOOLEAN:
    return "BOOLEAN";
  case msgpack::type::object_type::POSITIVE_INTEGER:
    return "POSITIVE_INTEGER";
  case msgpack::type::object_type::NEGATIVE_INTEGER:
    return "NEGATIVE_INTEGER";
  case msgpack::type::object_type::FLOAT32:
    return "FLOAT32";
  case msgpack::type::object_type::FLOAT64:
    return "FLOAT64";
  case msgpack::type::object_type::STR:
    return "STR";
  case msgpack::type::object_type::BIN:
    return "BIN";
  case msgpack::type::object_type::ARRAY:
    return "ARRAY";
  case msgpack::type::object_type::MAP:
    return "MAP";
  case msgpack::type::object_type::EXT:
    return "EXT";
  }
}

}; // namespace guile_shared
