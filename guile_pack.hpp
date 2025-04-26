#pragma once

#include "guile_object.hpp"
#include "guile_shared.hpp"
#include <cstdlib>
#include <stdexcept>

#include <msgpack.hpp>

namespace guile_pack {

// TODO: Replace with actual error type.
using pack_error = std::runtime_error;
namespace detail = guile_shared::detail;

constexpr const int8_t nil_ext_id = GUILE_PACK_EXT_ID_START;
constexpr const int8_t symbol_ext_id = GUILE_PACK_EXT_ID_START + 1;
constexpr const int8_t keyword_ext_id = GUILE_PACK_EXT_ID_START + 2;

using guile_type = guile_object::guile_type;

using flags_type = uint64_t;

enum class pack_flags : uint64_t {
  disable_symbol_extension = 1 << 0,
  disable_keyword_extension = 1 << 1,

  // exceptional circumstances
  override_unknowns = 1 << 8,
  unknown_is_nil = 1 << 9,
  unknown_is_panic = 1 << 10,
};

constexpr uint64_t operator&(pack_flags lhs, pack_flags rhs) noexcept {
  return static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs);
}

constexpr uint64_t operator&(uint64_t lhs, pack_flags rhs) noexcept {
  return lhs & static_cast<uint64_t>(rhs);
}

constexpr uint64_t operator|(pack_flags lhs, pack_flags rhs) noexcept {
  return static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs);
}

constexpr uint64_t operator|(uint64_t lhs, pack_flags rhs) noexcept {
  return lhs | static_cast<uint64_t>(rhs);
}

template <typename T>
inline void packDispatch(SCM value, guile_object::guile_type guile_t,
                         msgpack::packer<T> &packer, flags_type flags);

template <guile_type GT> struct GuilePacker : std::false_type {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    if ((flags & pack_flags::override_unknowns) == 0) {
      throw pack_error("cannot pack unknown value");
      return;
    } else {
      if ((flags & pack_flags::unknown_is_nil) != 0) {
        packer.pack_nil();
      } else if ((flags & pack_flags::unknown_is_panic) != 0) {
        guile_shared::panic();
      }
    }
  }
};

/// Declare a GuilePacker that passes the output of a single function to a
/// method of packer
#define GUILE_PACKER_SIMPLE(gtype, convert_fn, pack_fn)                        \
  template <> struct GuilePacker<gtype> : std::true_type {                     \
    template <typename T>                                                      \
    static inline void pack(SCM value, msgpack::packer<T> &packer,             \
                            flags_type flags) {                                \
      packer.pack_fn(convert_fn(value));                                       \
    }                                                                          \
  };

/// Declare a GuilePacker that only calls a method on the wrapped packer object.
#define GUILE_PACKER_TRIVIAL(gtype, pack_fn)                                   \
  template <> struct GuilePacker<gtype> : std::true_type {                     \
    template <typename T>                                                      \
    static inline void pack(SCM value, msgpack::packer<T> &packer,             \
                            flags_type flags) {                                \
      packer.pack_fn;                                                          \
    }                                                                          \
  };

GUILE_PACKER_TRIVIAL(guile_type::undefined, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::unspecified, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::eof, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::eol, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::nil, pack_nil());

GUILE_PACKER_SIMPLE(guile_type::fixed_num, scm_to_int64, pack_fix_int64);

GUILE_PACKER_SIMPLE(guile_type::real, scm_to_double, pack_double);

template <> struct GuilePacker<guile_type::boolean> : std::true_type {
  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    if (scm_is_true(value))
      packer.pack_true();
    else
      packer.pack_false();
  }
};

template <> struct GuilePacker<guile_type::pair> : std::true_type {
  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    std::deque<guile_object::SCMLazyTyped> typs{};
    SCM current = value, head, tail;
    while (SCM_CONSP(current)) {

      head = SCM_CAR(current);
      tail = SCM_CDR(current);
      typs.emplace_back(head);
      current = tail;
    }

    packer.pack_array(typs.size());
    for (auto t : typs) {
      ::guile_pack::packDispatch<T>(t.value(), t.type(), packer, flags);
    }
  }
};

template <> struct GuilePacker<guile_type::hashtable> : std::true_type {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    size_t len = SCM_HASHTABLE_N_ITEMS(value);

    packer.pack_map(len);

    SCM ref = SCM_HASHTABLE_VECTOR(value);
    size_t vlen = SCM_SIMPLE_VECTOR_LENGTH(ref);

    for (size_t i = 0; i < vlen; i++) {
      SCM current = SCM_SIMPLE_VECTOR_REF(ref, i);

      if (SCM_NULLP(current)) {
        continue;
      }
      assert(SCM_CONSP(current));
      packHTBucket(current, packer, flags);
    }
  }

private:
  template <typename T>
  static inline void packHTBucket(SCM value, msgpack::packer<T> &packer,
                                  flags_type flags) {
    std::deque<guile_object::SCMLazyTyped> typs{};
    SCM current = value, head, tail;
    while (SCM_CONSP(current)) {

      head = SCM_CAR(current);
      if (SCM_CONSP(head)) {
        typs.emplace_back(SCM_CAR(head));
        typs.emplace_back(SCM_CDR(head));
      }

      tail = SCM_CDR(current);
      current = tail;
    }

    for (auto t : typs) {
      ::guile_pack::packDispatch(t.value(), t.type(), packer, flags);
    }
  }
};

template <> struct GuilePacker<guile_type::string> {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    size_t len;
    std::unique_ptr<char, detail::malloc_deleter> obuf{};
    obuf.reset(scm_to_utf8_stringn(value, &len));
    packer.pack_str(len);
    packer.pack_str_body(obuf.get(), len);
  }
};

template <> struct GuilePacker<guile_type::symbol> {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    size_t len;
    std::unique_ptr<char, detail::malloc_deleter> obuf{};
    SCM sv = scm_symbol_to_string(value);
    obuf.reset(scm_to_utf8_stringn(sv, &len));

    if ((flags & pack_flags::disable_symbol_extension) == 0) {
      packer.pack_ext(len, symbol_ext_id);
      packer.pack_ext_body(obuf.get(), len);
    } else {
      packer.pack_str(len);
      packer.pack_str_body(obuf.get(), len);
    }
  }
};

template <> struct GuilePacker<guile_type::keyword> {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer,
                          flags_type flags) {
    size_t len;
    std::unique_ptr<char, detail::malloc_deleter> obuf{};
    SCM sv = scm_symbol_to_string(scm_keyword_to_symbol(value));
    obuf.reset(scm_to_utf8_stringn(sv, &len));

    if ((flags & pack_flags::disable_keyword_extension) == 0) {
      packer.pack_ext(len, keyword_ext_id);
      packer.pack_ext_body(obuf.get(), len);
    } else {
      packer.pack_str(len);
      packer.pack_str_body(obuf.get(), len);
    }
  }
};

// TODO: Implement packer for array using scm_array_get_handle
// TODO: Handle homogenous arrays with inline functions in
// libguile/array_handle.h

template <typename T>
inline void packDispatch(SCM value, guile_object::guile_type guile_t,
                         msgpack::packer<T> &packer, flags_type flags) {
  switch (guile_t) {

  case guile_type::boolean:
    GuilePacker<guile_type::boolean>::pack(value, packer, flags);
    break;
  case guile_type::undefined:
    GuilePacker<guile_type::undefined>::pack(value, packer, flags);
  case guile_type::unspecified:
    GuilePacker<guile_type::unspecified>::pack(value, packer, flags);
  case guile_type::eof:
    GuilePacker<guile_type::eof>::pack(value, packer, flags);
  case guile_type::eol:
    GuilePacker<guile_type::eol>::pack(value, packer, flags);
  case guile_type::nil:
    GuilePacker<guile_type::nil>::pack(value, packer, flags);
    break;
  case guile_type::fixed_num:
    GuilePacker<guile_type::fixed_num>::pack(value, packer, flags);
    break;
  case guile_type::real:
    GuilePacker<guile_type::real>::pack(value, packer, flags);
    break;
  case guile_type::pair:
    GuilePacker<guile_type::pair>::pack(value, packer, flags);
    break;
  case guile_type::struct_scm:
    GuilePacker<guile_type::struct_scm>::pack(value, packer, flags);
    break;
  case guile_type::closure:
    GuilePacker<guile_type::closure>::pack(value, packer, flags);
    break;
  case guile_type::symbol:
    GuilePacker<guile_type::symbol>::pack(value, packer, flags);
    break;
  case guile_type::variable:
    GuilePacker<guile_type::variable>::pack(value, packer, flags);
    break;
  case guile_type::vector:
    GuilePacker<guile_type::vector>::pack(value, packer, flags);
    break;
  case guile_type::wvect:
    GuilePacker<guile_type::wvect>::pack(value, packer, flags);
    break;
  case guile_type::string:
    GuilePacker<guile_type::string>::pack(value, packer, flags);
    break;
  case guile_type::hashtable:
    GuilePacker<guile_type::hashtable>::pack(value, packer, flags);
    break;
  case guile_type::pointer:
    GuilePacker<guile_type::pointer>::pack(value, packer, flags);
    break;
  case guile_type::fluid:
    GuilePacker<guile_type::fluid>::pack(value, packer, flags);
    break;
  case guile_type::stringbuf:
    GuilePacker<guile_type::stringbuf>::pack(value, packer, flags);
    break;
  case guile_type::dynamic_state:
    GuilePacker<guile_type::dynamic_state>::pack(value, packer, flags);
    break;
  case guile_type::frame:
    GuilePacker<guile_type::frame>::pack(value, packer, flags);
    break;
  case guile_type::keyword:
    GuilePacker<guile_type::keyword>::pack(value, packer, flags);
    break;
  case guile_type::atomic_box:
    GuilePacker<guile_type::atomic_box>::pack(value, packer, flags);
    break;
  case guile_type::syntax:
    GuilePacker<guile_type::syntax>::pack(value, packer, flags);
    break;
  case guile_type::values:
    GuilePacker<guile_type::values>::pack(value, packer, flags);
    break;
  case guile_type::program:
    GuilePacker<guile_type::program>::pack(value, packer, flags);
    break;
  case guile_type::vm_cont:
    GuilePacker<guile_type::vm_cont>::pack(value, packer, flags);
    break;
  case guile_type::bytevector:
    GuilePacker<guile_type::bytevector>::pack(value, packer, flags);
    break;
  case guile_type::weak_set:
    GuilePacker<guile_type::weak_set>::pack(value, packer, flags);
    break;
  case guile_type::weak_table:
    GuilePacker<guile_type::weak_table>::pack(value, packer, flags);
    break;
  case guile_type::array:
    GuilePacker<guile_type::array>::pack(value, packer, flags);
    break;
  case guile_type::bitvector:
    GuilePacker<guile_type::bitvector>::pack(value, packer, flags);
    break;
  case guile_type::smob:
    GuilePacker<guile_type::smob>::pack(value, packer, flags);
    break;
  case guile_type::port:
    GuilePacker<guile_type::port>::pack(value, packer, flags);
    break;
  case guile_type::invalid:
    GuilePacker<guile_type::invalid>::pack(value, packer, flags);
    break;
  }
}
} // namespace guile_pack
