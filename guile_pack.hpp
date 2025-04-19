#pragma once

#include "guile_object.hpp"
#include <msgpack.hpp>

#define GUILE_PACKER_SIMPLE(gtype, convert_fn, pack_fn)                        \
  template <> struct GuilePacker<gtype> : std::true_type {                     \
    template <typename T>                                                      \
    static inline void pack(SCM value, msgpack::packer<T> &packer) {           \
      packer.pack_fn(convert_fn(value));                                       \
    }                                                                          \
  };

#define GUILE_PACKER_TRIVIAL(gtype, pack_fn)                                   \
  template <> struct GuilePacker<gtype> : std::true_type {                     \
    template <typename T>                                                      \
    static inline void pack(SCM value, msgpack::packer<T> &packer) {           \
      packer.pack_fn;                                                          \
    }                                                                          \
  };

namespace guile_pack {

namespace detail {

struct malloc_deleter {
  void operator()(char *v) noexcept { ::free(v); }
};

} // namespace detail

static std::string_view display(msgpack::type::object_type typ) {
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

using guile_type = guile_object::guile_type;

template <typename T>
inline void packDispatch(SCM value, guile_object::guile_type guile_t,
                         msgpack::packer<T> &packer);

template <guile_type GT> struct GuilePacker : std::false_type {};

GUILE_PACKER_TRIVIAL(guile_type::undefined, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::unspecified, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::eof, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::eol, pack_nil());

GUILE_PACKER_TRIVIAL(guile_type::nil, pack_nil());

GUILE_PACKER_SIMPLE(guile_type::fixed_num, scm_to_int64, pack_fix_int64);

GUILE_PACKER_SIMPLE(guile_type::real, scm_to_double, pack_double);

template <> struct GuilePacker<guile_type::boolean> : std::true_type {
  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer) {
    if (scm_is_true(value))
      packer.pack_true();
    else
      packer.pack_false();
  }
};

template <> struct GuilePacker<guile_type::pair> : std::true_type {
  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer) {
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
      ::guile_pack::packDispatch<T>(t.value(), t.type(), packer);
    }
  }
};

template <> struct GuilePacker<guile_type::hashtable> : std::true_type {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer) {
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
      packHTBucket(current, packer);
    }
  }

private:
  template <typename T>
  static inline void packHTBucket(SCM value, msgpack::packer<T> &packer) {
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
      ::guile_pack::packDispatch(t.value(), t.type(), packer);
    }
  }
};

template <> struct GuilePacker<guile_type::string> {

  template <typename T>
  static inline void pack(SCM value, msgpack::packer<T> &packer) {
    size_t len;
    std::unique_ptr<char, detail::malloc_deleter> obuf{};
    obuf.reset(scm_to_utf8_stringn(value, &len));
    packer.pack_str(len);
    packer.pack_str_body(obuf.get(), len);
  }
};

template <typename T>
inline void packDispatch(SCM value, guile_object::guile_type guile_t,
                         msgpack::packer<T> &packer) {
  while (true) {
    switch (guile_t) {

    case guile_type::boolean:
      GuilePacker<guile_type::boolean>::pack(value, packer);
      break;
    case guile_type::undefined:
      GuilePacker<guile_type::undefined>::pack(value, packer);
    case guile_type::unspecified:
      GuilePacker<guile_type::unspecified>::pack(value, packer);
    case guile_type::eof:
      GuilePacker<guile_type::eof>::pack(value, packer);
    case guile_type::eol:
      GuilePacker<guile_type::eol>::pack(value, packer);
    case guile_type::nil:
      GuilePacker<guile_type::nil>::pack(value, packer);
      break;
    case guile_type::fixed_num:
      GuilePacker<guile_type::fixed_num>::pack(value, packer);
      break;
    case guile_type::real:
      GuilePacker<guile_type::real>::pack(value, packer);
      break;
    case guile_type::pair:
      GuilePacker<guile_type::pair>::pack(value, packer);
      break;
    case guile_type::struct_scm:
      break;
    case guile_type::closure:
      break;
    case guile_type::symbol:
      break;
    case guile_type::variable:
      break;
    case guile_type::vector:
      break;
    case guile_type::wvect:
      break;
    case guile_type::string:
      GuilePacker<guile_type::string>::pack(value, packer);
      break;
    case guile_type::hashtable:
      GuilePacker<guile_type::hashtable>::pack(value, packer);
      break;
    case guile_type::pointer:
      break;
    case guile_type::fluid:
      break;
    case guile_type::stringbuf:
      break;
    case guile_type::dynamic_state:
      break;
    case guile_type::frame:
      break;
    case guile_type::keyword:
      break;
    case guile_type::atomic_box:
      break;
    case guile_type::syntax:
      break;
    case guile_type::values:
      break;
    case guile_type::program:
      break;
    case guile_type::vm_cont:
      break;
    case guile_type::bytevector:
      break;
    case guile_type::weak_set:
      break;
    case guile_type::weak_table:
      break;
    case guile_type::array:
      break;
    case guile_type::bitvector:
      break;
    case guile_type::smob:
      break;
    case guile_type::port:
      break;
    case guile_type::invalid:
      break;
    }

    break;
  }
}
} // namespace guile_pack
