#include "guile_object.hpp"
#include "guile_pack.hpp"
#include "guile_unpack.hpp"
#include "msgpack/v3/object_fwd_decl.hpp"
#include <iostream>
#include <msgpack.hpp>
#include <sys/types.h>

static msgpack::sbuffer packScm(SCM value, uint64_t flags) {
  msgpack::sbuffer buf;
  msgpack::packer<msgpack::sbuffer> packer(buf);

  guile_pack::packDispatch(value, guile_object::guile_type_of(value), packer,
                           flags);

  return buf;
}

static void debugMpack(const msgpack::sbuffer &buffer) {
  msgpack::object_handle result;
  msgpack::unpack(result, buffer.data(), buffer.size());
  auto ob = result.get();
  std::cout << "OBJ: " << ob << "|TYP: " << guile_pack::display(ob.type)
            << std::endl;

  // TODO
  if (ob.type != msgpack::type::EXT) {
    auto v = guile_unpack::unpackDispatch(result.get(), 0);

    scm_write(v, scm_current_output_port());
  }
}

static void dumpScm(std::string ident, SCM v, uint64_t flags = 0) {
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
  debugMpack(packScm(v, flags));
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
  dumpScm("KEYWORD", scm_c_eval_string("#:test"));
  dumpScm("symbol", scm_c_eval_string("'test"));
  dumpScm("HASHTABLE", scm_c_eval_string(R"END(
(let ((ht (make-hash-table)))
  (hash-set! ht "test" 1)
  (hash-set! ht "test2" 2)
  (hash-set! ht "test3" 3)
  ht)
)END"));

  dumpScm("INVALID", scm_current_output_port(),
          guile_pack::pack_flags::override_unknowns |
              guile_pack::pack_flags::unknown_is_nil);
  // dumpScm("INVALID", scm_current_output_port(),
  //         static_cast<uint64_t>(guile_pack::pack_flags::unknown_is_panic) |
  //             static_cast<uint64_t>(guile_pack::pack_flags::override_unknowns));
  return 0;
}
