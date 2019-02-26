/*
 * MIT License
 *
 * Copyright (c) 2018 Nick Wanninger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/dict.h>
#include <cedar/object/keyword.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/vector.h>
#include <cedar/objtype.h>
#include <cedar/vm/binding.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mutex>


using namespace cedar;


#define ERROR_IF_ARGS_PASSED_IS(name, op, c)                            \
  if (argc op c)                                                        \
    throw cedar::make_exception("(" #name " ...) requires ", c, " arg", \
                                (c == 1 ? "" : "s"), "given ", argc);


cedar_binding(bits_or) {
  ERROR_IF_ARGS_PASSED_IS("bit-or", <, 2);
  i64 res = argv[0].to_int();
  for (int i = 1; i < argc; i++) res |= argv[i].to_int();
  return res;
}

cedar_binding(bits_and) {
  ERROR_IF_ARGS_PASSED_IS("bit-and", <, 2);
  i64 res = argv[0].to_int();
  for (int i = 1; i < argc; i++) res &= argv[i].to_int();
  return res;
}

cedar_binding(bits_xor) {
  ERROR_IF_ARGS_PASSED_IS("bit-xor", !=, 2);
  i64 res = argv[0].to_int() & argv[1].to_int();
  return res;
}

cedar_binding(bits_shift_left) {
  ERROR_IF_ARGS_PASSED_IS("bit-shift-left", !=, 2);
  i64 res = argv[0].to_int() << argv[1].to_int();
  return res;
}

cedar_binding(bits_shift_right) {
  ERROR_IF_ARGS_PASSED_IS("bit-shift-right", !=, 2);
  i64 res = argv[0].to_int() >> argv[1].to_int();
  return res;
}


cedar_binding(bits_shift_right_logic) {
  ERROR_IF_ARGS_PASSED_IS("bit-shift-right-logic", !=, 2);
  i64 res = ((u64)argv[0].to_int()) >> (u64)argv[1].to_int();
  return res;
}


void bind_bits(void) {
  module *mod = new module("bits");

  mod->def("or", bits_or);
  mod->def("and", bits_and);
  mod->def("xor", bits_xor);
  mod->def("shift-left", bits_shift_left);
  mod->def("shift-right", bits_shift_right);
  mod->def("shift-right-logic", bits_shift_right_logic);

  define_builtin_module("bits", mod);
}
