#include "guile_object.hpp"
#include "guile_pack.hpp"
#include "guile_unpack.hpp"

extern "C" void guile_msgpack_module_init() noexcept;

SCM guile_packScmToBytevector(SCM input) noexcept {
  msgpack::sbuffer buf;
  msgpack::packer<msgpack::sbuffer> packer(buf);

  guile_pack::packDispatch(input, guile_object::guile_type_of(input), packer,
                           guile_pack::pack_flags::override_unknowns | guile_pack::pack_flags::unknown_is_nil);

  SCM bv = scm_c_make_bytevector(buf.size());

  memcpy(SCM_BYTEVECTOR_CONTENTS(bv), buf.data(), buf.size());
  return bv;
}

SCM guile_unpackScmFromBytevector(SCM input) noexcept {
  msgpack::object_handle result;
  msgpack::unpack(result, (const char *)SCM_BYTEVECTOR_CONTENTS(input),
                  SCM_BYTEVECTOR_LENGTH(input));
  return guile_unpack::unpackDispatch(result.get(), 0);
}

extern "C" void guile_msgpack_module_init() noexcept {
  scm_c_define_gsubr("msgpack-pack-scm", 1, 0, 0, (void*)&guile_packScmToBytevector);
  scm_c_define_gsubr("msgpack-unpack-scm", 1, 0, 0, (void*)&guile_unpackScmFromBytevector);
}
