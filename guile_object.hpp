#pragma once

#include <string_view>

#define SCM_DEBUG_TYPING_STRICTNESS 0
#include <libguile.h>

#define GOBJ_CASE_OF(e) static_cast<unsigned char>(e)

namespace guile_object {

constexpr bool is_immediate(SCM v) noexcept { return SCM_IMP(v); }
constexpr bool is_heap(SCM v) noexcept { return SCM_HEAP_OBJECT_P(v); }

enum class tc2 : unsigned char { heap_or_nonimmediate = 0, small_int = 0b10 };

enum class tc3 : unsigned char {
  heap_object = 0b000,
  even_small_int = 0b010,
  non_int_immediate = 0b100,
  odd_small_int = 0b110,
};

enum class heap_tc3 : unsigned char {
  heap_cons =
      0b000, //  the cell belongs to a pair with a heap object in its car.
  heap_struct = 0b001,         //  the cell belongs to a struct
  heap_cons_even_int = 0b010,  //  the cell belongs to a pair with an even short
                               //  integer in its car.
  heap_closure = 0b011,        //  the cell belongs to a closure
  heap_cons_imm_other = 0b100, //  the cell belongs to a pair with a non-integer
                               //  immediate in its car.
  heap_object = 0b101,         //  the cell belongs to some other heap object.
  heap_cons_odd_int = 0b110,   //  the cell belongs to a pair with an odd short
                               //  integer in its car.
  heap_object2 = 0b111,        //  the cell belongs to some other heap object.
};

enum class tc8 {
  flag = scm_tc3_imm24 + 0x00,
  character = scm_tc3_imm24 + 0x08,
};

// Copied from libguile/scm.h
enum class tc7 : unsigned char {
  symbol = 0x05,
  variable = 0x07,
  vector = 0x0d,
  wvect = 0x0f,
  string = 0x15,
  number = 0x17,
  hashtable = 0x1d,
  pointer = 0x1f,
  fluid = 0x25,
  stringbuf = 0x27,
  dynamic_state = 0x2d,
  frame = 0x2f,
  keyword = 0x35,
  atomic_box = 0x37,
  syntax = 0x3d,
  values = 0x3f,
  program = 0x45,
  vm_cont = 0x47,
  bytevector = 0x4d,
  unused_4f = 0x4f,
  weak_set = 0x55,
  weak_table = 0x57,
  array = 0x5d,
  bitvector = 0x5f,
  unused_65 = 0x65,
  unused_67 = 0x67,
  unused_6d = 0x6d,
  unused_6f = 0x6f,
  unused_75 = 0x75,
  smob = 0x77,
  port = 0x7d,
  unused_7f = 0x7f
};

enum class immediate_type { not_immediate, small_int, other, invalid };

constexpr immediate_type immediate_type_of(SCM v) noexcept {
  switch (SCM_ITAG3(v)) {

  case GOBJ_CASE_OF(tc3::heap_object):
    return immediate_type::not_immediate;

  case GOBJ_CASE_OF(tc3::even_small_int):
  case GOBJ_CASE_OF(tc3::odd_small_int):
    return immediate_type::small_int;

  case GOBJ_CASE_OF(tc3::non_int_immediate):
    return immediate_type::other;
  default:
    return immediate_type::invalid;
  };
}

enum class heap_tc3_type {
  pair_heap_car,
  struct_object,
  pair_even_int,
  closure,
  pair_other_immediate,
  pair_odd_int,
  other_heap
};

inline heap_tc3_type heap_tc3_type_of(SCM v) noexcept {
  switch (SCM_TYP3(v)) {
  case GOBJ_CASE_OF(heap_tc3::heap_cons):
    return heap_tc3_type::pair_heap_car;

  case GOBJ_CASE_OF(heap_tc3::heap_struct):
    return heap_tc3_type::struct_object;

  case GOBJ_CASE_OF(heap_tc3::heap_cons_even_int):
    return heap_tc3_type::pair_even_int;

  case GOBJ_CASE_OF(heap_tc3::heap_closure):
    return heap_tc3_type::closure;

  case GOBJ_CASE_OF(heap_tc3::heap_cons_imm_other):
    return heap_tc3_type::pair_other_immediate;

  case GOBJ_CASE_OF(heap_tc3::heap_cons_odd_int):
    return heap_tc3_type::pair_odd_int;

    // case GOBJ_CASE_OF(heap_tc3::heap_object):
    // case GOBJ_CASE_OF(heap_tc3::heap_object2):
  default:
    return heap_tc3_type::other_heap;
  }
  return heap_tc3_type::other_heap;
}

enum class heap_type {
  pair,
  struct_scm,
  closure,
  symbol,
  variable,
  vector,
  wvect,
  string,
  number,
  hashtable,
  pointer,
  fluid,
  stringbuf,
  dynamic_state,
  frame,
  keyword,
  atomic_box,
  syntax,
  values,
  program,
  vm_cont,
  bytevector,
  weak_set,
  weak_table,
  array,
  bitvector,
  smob,
  port,
  unused,
  invalid
};

constexpr std::string_view display(heap_type ht) noexcept {
  switch (ht) {
    // clang-format off
      case heap_type::pair: return "pair";
      case heap_type::struct_scm: return "struct_scm";
      case heap_type::closure: return "closure";
      case heap_type::symbol: return "symbol";
      case heap_type::variable: return "variable";
      case heap_type::vector: return "vector";
      case heap_type::wvect: return "wvect";
      case heap_type::string: return "string";
      case heap_type::number: return "number";
      case heap_type::hashtable: return "hashtable";
      case heap_type::pointer: return "pointer";
      case heap_type::fluid: return "fluid";
      case heap_type::stringbuf: return "stringbuf";
      case heap_type::dynamic_state: return "dynamic_state";
      case heap_type::frame: return "frame";
      case heap_type::keyword: return "keyword";
      case heap_type::atomic_box: return "atomic_box";
      case heap_type::syntax: return "syntax";
      case heap_type::values: return "values";
      case heap_type::program: return "program";
      case heap_type::vm_cont: return "vm_cont";
      case heap_type::bytevector: return "bytevector";
      case heap_type::weak_set: return "weak_set";
      case heap_type::weak_table: return "weak_table";
      case heap_type::array: return "array";
      case heap_type::bitvector: return "bitvector";
      case heap_type::smob: return "smob";
      case heap_type::port: return "port";
      case heap_type::unused: return "unused";
      case heap_type::invalid: return "invalid";
    // clang-format on
  }
}

inline heap_type heap_type_of(SCM v) noexcept {
  SCM cellType = SCM_CELL_TYPE(v);

  // TC3 in cell
  switch (0b00000111 & cellType) {
  case GOBJ_CASE_OF(heap_tc3::heap_cons):
  case GOBJ_CASE_OF(heap_tc3::heap_cons_even_int):
  case GOBJ_CASE_OF(heap_tc3::heap_cons_imm_other):
  case GOBJ_CASE_OF(heap_tc3::heap_cons_odd_int):
    return heap_type::pair;

  case GOBJ_CASE_OF(heap_tc3::heap_struct):
    return heap_type::struct_scm;

  case GOBJ_CASE_OF(heap_tc3::heap_closure):
    return heap_type::closure;

  default:
    switch (0b01111111 & cellType) {
    case GOBJ_CASE_OF(tc7::symbol):
      return heap_type::symbol;

    case GOBJ_CASE_OF(tc7::variable):
      return heap_type::variable;

    case GOBJ_CASE_OF(tc7::vector):
      return heap_type::vector;

    case GOBJ_CASE_OF(tc7::wvect):
      return heap_type::wvect;

    case GOBJ_CASE_OF(tc7::string):
      return heap_type::string;

    case GOBJ_CASE_OF(tc7::number):
      return heap_type::number;

    case GOBJ_CASE_OF(tc7::hashtable):
      return heap_type::hashtable;

    case GOBJ_CASE_OF(tc7::pointer):
      return heap_type::pointer;

    case GOBJ_CASE_OF(tc7::fluid):
      return heap_type::fluid;

    case GOBJ_CASE_OF(tc7::stringbuf):
      return heap_type::stringbuf;

    case GOBJ_CASE_OF(tc7::dynamic_state):
      return heap_type::dynamic_state;

    case GOBJ_CASE_OF(tc7::frame):
      return heap_type::frame;

    case GOBJ_CASE_OF(tc7::keyword):
      return heap_type::keyword;

    case GOBJ_CASE_OF(tc7::atomic_box):
      return heap_type::atomic_box;

    case GOBJ_CASE_OF(tc7::syntax):
      return heap_type::syntax;

    case GOBJ_CASE_OF(tc7::values):
      return heap_type::values;

    case GOBJ_CASE_OF(tc7::program):
      return heap_type::program;

    case GOBJ_CASE_OF(tc7::vm_cont):
      return heap_type::vm_cont;

    case GOBJ_CASE_OF(tc7::bytevector):
      return heap_type::bytevector;

    case GOBJ_CASE_OF(tc7::weak_set):
      return heap_type::weak_set;

    case GOBJ_CASE_OF(tc7::weak_table):
      return heap_type::weak_table;

    case GOBJ_CASE_OF(tc7::array):
      return heap_type::array;

    case GOBJ_CASE_OF(tc7::bitvector):
      return heap_type::bitvector;

    case GOBJ_CASE_OF(tc7::smob):
      return heap_type::smob;

    case GOBJ_CASE_OF(tc7::port):
      return heap_type::port;

    case GOBJ_CASE_OF(tc7::unused_4f):
    case GOBJ_CASE_OF(tc7::unused_65):
    case GOBJ_CASE_OF(tc7::unused_67):
    case GOBJ_CASE_OF(tc7::unused_6d):
    case GOBJ_CASE_OF(tc7::unused_6f):
    case GOBJ_CASE_OF(tc7::unused_75):
    case GOBJ_CASE_OF(tc7::unused_7f):
      return heap_type::unused;
    }
  }

  return heap_type::invalid;
}

enum class guile_type {
  boolean,
  nil,
  eol,
  eof,
  undefined,
  unspecified,
  fixed_num,
  real,
  pair,
  struct_scm,
  closure,
  symbol,
  variable,
  vector,
  wvect,
  string,
  hashtable,
  pointer,
  fluid,
  stringbuf,
  dynamic_state,
  frame,
  keyword,
  atomic_box,
  syntax,
  values,
  program,
  vm_cont,
  bytevector,
  weak_set,
  weak_table,
  array,
  bitvector,
  smob,
  port,
  invalid
};

constexpr std::string_view display(guile_type gt) {
  using enum guile_type;
  switch (gt) {
    // clang-format off
      case boolean:       return "boolean";
      case eol:           return "eol";
      case eof:           return "eof";
      case nil:           return "nil";
      case undefined:     return "undefined";
      case unspecified:   return "unspecified";
      case fixed_num:     return "fixed_num";
      case real:          return "real";
      case pair:          return "pair";
      case struct_scm:    return "struct_scm";
      case closure:       return "closure";
      case symbol:        return "symbol";
      case variable:      return "variable";
      case vector:        return "vector";
      case wvect:         return "wvect";
      case string:        return "string";
      case hashtable:     return "hashtable";
      case pointer:       return "pointer";
      case fluid:         return "fluid";
      case stringbuf:     return "stringbuf";
      case dynamic_state: return "dynamic_state";
      case frame:         return "frame";
      case keyword:       return "keyword";
      case atomic_box:    return "atomic_box";
      case syntax:        return "syntax";
      case values:        return "values";
      case program:       return "program";
      case vm_cont:       return "vm_cont";
      case bytevector:    return "bytevector";
      case weak_set:      return "weak_set";
      case weak_table:    return "weak_table";
      case array:         return "array";
      case bitvector:     return "bitvector";
      case smob:          return "smob";
      case port:          return "port";
      case invalid:       return "invalid";
    // clang-format on
  }
}

inline guile_type guile_type_of(SCM v) noexcept {
  switch (immediate_type_of(v)) {
  case immediate_type::small_int:
    return guile_type::fixed_num;
  case immediate_type::invalid:
    return guile_type::invalid;
  case immediate_type::other:
    switch (v) {
    case SCM_BOOL_F:
    case SCM_BOOL_T:
      return guile_type::boolean;
    case SCM_EOL:
      return guile_type::eol;
    case SCM_EOF_VAL:
      return guile_type::eof;
    case SCM_UNSPECIFIED:
      return guile_type::unspecified;
    case SCM_UNDEFINED:
      return guile_type::undefined;
    default:
      return guile_type::invalid;
    }
  default:
    switch (heap_type_of(v)) {
      // clang-format off
    case heap_type::pair:          return guile_type::pair;
    case heap_type::struct_scm:    return guile_type::struct_scm;
    case heap_type::closure:       return guile_type::closure;
    case heap_type::symbol:        return guile_type::symbol;
    case heap_type::variable:      return guile_type::variable;
    case heap_type::vector:        return guile_type::vector;
    case heap_type::wvect:         return guile_type::wvect;
    case heap_type::string:        return guile_type::string;
    case heap_type::hashtable:     return guile_type::hashtable;
    case heap_type::pointer:       return guile_type::pointer;
    case heap_type::fluid:         return guile_type::fluid;
    case heap_type::stringbuf:     return guile_type::stringbuf;
    case heap_type::dynamic_state: return guile_type::dynamic_state;
    case heap_type::frame:         return guile_type::frame;
    case heap_type::keyword:       return guile_type::keyword;
    case heap_type::atomic_box:    return guile_type::atomic_box;
    case heap_type::syntax:        return guile_type::syntax;
    case heap_type::values:        return guile_type::values;
    case heap_type::program:       return guile_type::program;
    case heap_type::vm_cont:       return guile_type::vm_cont;
    case heap_type::bytevector:    return guile_type::bytevector;
    case heap_type::weak_set:      return guile_type::weak_set;
    case heap_type::weak_table:    return guile_type::weak_table;
    case heap_type::array:         return guile_type::array;
    case heap_type::bitvector:     return guile_type::bitvector;
    case heap_type::smob:          return guile_type::smob;
    case heap_type::port:          return guile_type::port;
    case heap_type::unused:        return guile_type::invalid;
    case heap_type::invalid:       return guile_type::invalid;
      // clang-format on
    case heap_type::number:
      if (SCM_INEXACTP(v)) {
        return guile_type::real;
      } else {
        return guile_type::fixed_num;
      }
      // TODO: Add complex numbers (maybe ;-;)
    }
  }

  return guile_type::invalid;
}



}; // namespace guile_object

#undef GOBJ_CASE_OF
