#include "guile_object.hpp"
#include "libguile/boolean.h"
#include "libguile/hashtab.h"
#include "libguile/numbers.h"
#include "libguile/pairs.h"
#include "libguile/ports.h"
#include "libguile/scm.h"
#include "libguile/strings.h"
#include "libguile/vectors.h"
#include "msgpack/v3/adaptor/adaptor_base_decl.hpp"
#include <deque>
#include <iostream>
#include <memory>
#include <msgpack.hpp>
#include <sys/types.h>

struct malloc_deleter {
  void operator()(char *v) noexcept { ::free(v); }
};

class SCMLazyTyped {

public:
  constexpr SCMLazyTyped(SCM val) noexcept : value_(val) {};

  constexpr SCM value() const & noexcept { return value_; }
  constexpr guile_object::guile_type type() const & noexcept {
    return do_resolve();
  }

private:
  inline guile_object::guile_type do_resolve() const {
    if (!has_resolved) {
      has_resolved = true;
      typ_ = guile_object::guile_type_of(value_);
    }

    return typ_;
  }

  SCM value_;
  mutable bool has_resolved = false;
  mutable guile_object::guile_type typ_ = guile_object::guile_type::unspecified;
};

inline void packCons(SCM value, msgpack::packer<msgpack::sbuffer> &packer);
inline void packMap(SCM value, msgpack::packer<msgpack::sbuffer> &packer);

inline void packString(SCM value, msgpack::packer<msgpack::sbuffer> &packer) {
  size_t len;
  std::unique_ptr<char, ::malloc_deleter> obuf{};
  obuf.reset(scm_to_utf8_stringn(value, &len));
  std::cout << std::dec << (uint)len << std::endl;
  packer.pack_str(len);
  packer.pack_str_body(obuf.get(), len);
}

inline void packInternal(SCM value, guile_object::guile_type guile_t,
                         msgpack::packer<msgpack::sbuffer> &packer) {
  while (true) {
    // auto guile_t = guile_object::guile_type_of(value);
    // std::cout << guile_object::display(guile_t) << std::endl;

    switch (guile_t) {

    case guile_object::guile_type::boolean:
      if (scm_is_true(value)) {
        packer.pack_true();
      } else {
        packer.pack_false();
      }
      break;
    case guile_object::guile_type::undefined:
    case guile_object::guile_type::unspecified:
    case guile_object::guile_type::eof:
    case guile_object::guile_type::eol:
    case guile_object::guile_type::nil:
      packer.pack_nil();
      break;
    case guile_object::guile_type::fixed_num:
      packer.pack_fix_int64(scm_to_int64(value));
      break;
    case guile_object::guile_type::real:
      packer.pack_float(scm_to_double(value));
      break;
    case guile_object::guile_type::pair:
      ::packCons(value, packer);
      break;
    case guile_object::guile_type::struct_scm:
      break;
    case guile_object::guile_type::closure:
      break;
    case guile_object::guile_type::symbol:
      break;
    case guile_object::guile_type::variable:
      break;
    case guile_object::guile_type::vector:
      break;
    case guile_object::guile_type::wvect:
      break;
    case guile_object::guile_type::string:
      ::packString(value, packer);
      break;
    case guile_object::guile_type::hashtable:
      ::packMap(value, packer);
      break;
    case guile_object::guile_type::pointer:
      break;
    case guile_object::guile_type::fluid:
      break;
    case guile_object::guile_type::stringbuf:
      break;
    case guile_object::guile_type::dynamic_state:
      break;
    case guile_object::guile_type::frame:
      break;
    case guile_object::guile_type::keyword:
      break;
    case guile_object::guile_type::atomic_box:
      break;
    case guile_object::guile_type::syntax:
      break;
    case guile_object::guile_type::values:
      break;
    case guile_object::guile_type::program:
      break;
    case guile_object::guile_type::vm_cont:
      break;
    case guile_object::guile_type::bytevector:
      break;
    case guile_object::guile_type::weak_set:
      break;
    case guile_object::guile_type::weak_table:
      break;
    case guile_object::guile_type::array:
      break;
    case guile_object::guile_type::bitvector:
      break;
    case guile_object::guile_type::smob:
      break;
    case guile_object::guile_type::port:
      break;
    case guile_object::guile_type::invalid:
      break;
    }

    break;
  }
}

static msgpack::sbuffer packScm(SCM value) {
  msgpack::sbuffer buf;
  msgpack::packer<msgpack::sbuffer> packer(buf);

  packInternal(value, guile_object::guile_type_of(value), packer);

  return buf;
}

inline void packCons(SCM value, msgpack::packer<msgpack::sbuffer> &packer) {
  std::deque<SCMLazyTyped> typs{};
  SCM current = value, head, tail;
  while (SCM_CONSP(current)) {

    head = SCM_CAR(current);
    tail = SCM_CDR(current);
    typs.emplace_back(head);
    current = tail;
  }

  packer.pack_array(typs.size());
  for (auto t : typs) {
    packInternal(t.value(), t.type(), packer);
  }
}

inline void packConsBare(SCM value, msgpack::packer<msgpack::sbuffer> &packer) {
  std::deque<SCMLazyTyped> typs{};
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
    // scm_write(t.value(), scm_current_output_port());
    packInternal(t.value(), t.type(), packer);
  }
}

inline void packMap(SCM value, msgpack::packer<msgpack::sbuffer> &packer) {
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
    packConsBare(current, packer);
  }
}

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

static void debugMpack(const msgpack::sbuffer &buffer) {
  msgpack::object_handle result;
  msgpack::unpack(result, buffer.data(), buffer.size());
  auto ob = result.get();
  std::cout << "OBJ: " << ob << "|TYP: " << display(ob.type) << std::endl;
}

static void dumpScm(std::string ident, SCM v) {
  // clang-format off
  std::cout 
    << std::hex
    << ident << " " << (uintptr_t)v
    << " "
    << guile_object::display(guile_object::guile_type_of(v))
    << "\nIS_IMMEDIATE " << guile_object::is_immediate(v)
    << "\nIS_HEAP " << guile_object::is_heap(v)
    << "\nSCM_ITAG3 " << SCM_ITAG3(v) 
    << "\nSCM_ITAG7 " << SCM_ITAG7(v);
  // clang-format on
  if (guile_object::is_heap(v)) {
    // clang-format off
  std::cout 
    << "\nSCM_TYP7 " << SCM_TYP7(v)
    << std::endl;
    // clang-format on
  }
  std::cout << "\nMpack output ";
  debugMpack(packScm(v));
  std::cout << "\n" << std::endl;
}

int main(int argc, char **argv) {
  scm_init_guile();

  std::cout << "Start" << std::endl;

  dumpScm("SCM_BOOL_F", SCM_BOOL_F);
  dumpScm("SCM_BOOL_T", SCM_BOOL_T);
  dumpScm("SCM_EOL", SCM_EOL);
  dumpScm("SCM_EOF_VAL", SCM_EOF_VAL);
  dumpScm("INT", scm_from_int(123));
  dumpScm("REAL", scm_from_double(123.456));
  dumpScm("CONS (#f)", scm_cons(SCM_BOOL_F, SCM_EOL));
  dumpScm("CONS (#f 345)", scm_cons(SCM_BOOL_F, scm_from_int(345)));
  dumpScm("CONS", scm_c_eval_string("'(1 2)"));
  dumpScm("STRING", scm_from_utf8_string("THIS IS A TEST"));

  dumpScm("HASHTABLE", scm_c_eval_string(R"END(
(let ((ht (make-hash-table)))
  (hash-set! ht "test" 1)
  (hash-set! ht "test2" 2)
  (hash-set! ht "test3" 3)
  ht)
)END"));

  return 0;
}
