#pragma once

#include "guile_object.hpp"
#include "guile_shared.hpp"
#include "libguile/numbers.h"
#include "libguile/pairs.h"
#include "libguile/symbols.h"
#include <iostream>
#include <msgpack.hpp>
#include <type_traits>

namespace guile_unpack {

using unpack_error = std::runtime_error;
using object_handle_t = const msgpack::object;
using flags_t = uint64_t;
using scm_t = SCM;
inline scm_t unpackDispatch(object_handle_t &object, flags_t flags);

enum class unpack_flags : uint64_t {
  disable_symbol_extension = 1 << 0,
  disable_keyword_extension = 1 << 1,
};

constexpr uint64_t operator&(unpack_flags lhs, unpack_flags rhs) noexcept {
  return static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs);
}

constexpr uint64_t operator&(uint64_t lhs, unpack_flags rhs) noexcept {
  return lhs & static_cast<uint64_t>(rhs);
}

constexpr uint64_t operator|(unpack_flags lhs, unpack_flags rhs) noexcept {
  return static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs);
}

constexpr uint64_t operator|(uint64_t lhs, unpack_flags rhs) noexcept {
  return lhs | static_cast<uint64_t>(rhs);
}

template <msgpack::type::object_type T> struct GuileUnpacker : std::false_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    throw new unpack_error("cannot unpack object of unknown type");
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::NIL> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    return SCM_EOL;
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::BOOLEAN> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::BOOLEAN);
    if (handle.via.boolean) {
      return SCM_BOOL_T;
    } else {
      return SCM_BOOL_F;
    }
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::POSITIVE_INTEGER>
    : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::POSITIVE_INTEGER);
    return scm_from_unsigned_integer(handle.via.u64);
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::NEGATIVE_INTEGER>
    : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::NEGATIVE_INTEGER);
    return scm_from_signed_integer(handle.via.u64);
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::FLOAT32> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::FLOAT32);
    return scm_from_double(handle.via.f64);
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::FLOAT64> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::FLOAT64);
    return scm_from_double(handle.via.f64);
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::STR> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::STR);
    auto data = handle.via.str;

    return scm_from_utf8_stringn(data.ptr, data.size);
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::BIN> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::BIN);
    auto data = handle.via.bin;

    scm_t bv = scm_c_make_bytevector(data.size);

    memcpy(SCM_BYTEVECTOR_CONTENTS(bv), data.ptr, data.size);
    return bv;
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::ARRAY> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::ARRAY);
    auto data = handle.via.array;
    scm_t result = scm_c_make_vector(data.size, SCM_BOOL_F);

    for (uint32_t i = 0; i < data.size; i++) {
      scm_c_vector_set_x(result, i, unpackDispatch(data.ptr[i], flags));
    }
    return result;
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::MAP> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::MAP);
    auto data = handle.via.map;
    scm_t result = scm_c_make_hash_table(data.size);
    for (uint32_t i = 0; i < data.size; i++) {
      scm_hash_set_x(result, unpackDispatch(data.ptr[i].key, flags),
                     unpackDispatch(data.ptr[i].val, flags));
    }
    return result;
  }
};

// TODO
template <>
struct GuileUnpacker<msgpack::type::object_type::EXT> : std::true_type {
  static inline scm_t unpack(object_handle_t &handle, flags_t flags) {
    assert(handle.type == msgpack::type::object_type::EXT);
    auto data = handle.via.ext;

    switch (data.type()) {
    case guile_shared::symbol_ext_id:
      if ((flags & unpack_flags::disable_symbol_extension) == 0) {
        return scm_string_to_symbol(scm_from_utf8_stringn(data.data(), data.size));
      } else {
        return scm_from_utf8_stringn(data.data(), data.size);
      }
    case guile_shared::keyword_ext_id:
      if ((flags & unpack_flags::disable_symbol_extension) == 0) {
        return scm_symbol_to_keyword(
            scm_string_to_symbol(scm_from_utf8_stringn(data.data(), data.size)));
      } else {
        return scm_from_utf8_stringn(data.data(), data.size);
      }
    case guile_shared::nil_ext_id:
      return SCM_EOL;
    default:
      scm_t bvec = scm_c_make_bytevector(data.size);
      memcpy(SCM_BYTEVECTOR_CONTENTS(bvec), data.ptr, data.size);
      return scm_cons(scm_from_int8(data.type()), bvec);
    }
  }
};

inline scm_t unpackDispatch(object_handle_t &object, flags_t flags) {
  switch (object.type) {

  case msgpack::type::NIL:
    return GuileUnpacker<msgpack::type::NIL>::unpack(object, flags);
  case msgpack::type::BOOLEAN:
    return GuileUnpacker<msgpack::type::BOOLEAN>::unpack(object, flags);
  case msgpack::type::POSITIVE_INTEGER:
    return GuileUnpacker<msgpack::type::POSITIVE_INTEGER>::unpack(object,
                                                                  flags);
  case msgpack::type::NEGATIVE_INTEGER:
    return GuileUnpacker<msgpack::type::NEGATIVE_INTEGER>::unpack(object,
                                                                  flags);
  case msgpack::type::FLOAT32:
    return GuileUnpacker<msgpack::type::FLOAT32>::unpack(object, flags);
  case msgpack::type::FLOAT64:
    return GuileUnpacker<msgpack::type::FLOAT64>::unpack(object, flags);
  case msgpack::type::STR:
    return GuileUnpacker<msgpack::type::STR>::unpack(object, flags);
  case msgpack::type::BIN:
    return GuileUnpacker<msgpack::type::BIN>::unpack(object, flags);
  case msgpack::type::ARRAY:
    return GuileUnpacker<msgpack::type::ARRAY>::unpack(object, flags);
  case msgpack::type::MAP:
    return GuileUnpacker<msgpack::type::MAP>::unpack(object, flags);
  case msgpack::type::EXT:
    return GuileUnpacker<msgpack::type::EXT>::unpack(object, flags);
  }
}

}; // namespace guile_unpack
