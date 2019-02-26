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

static ref merge_here(int argc, ref *argv, call_context *ctx) {
  module *target = ctx->mod;

  if (argc != 1) {
    throw cedar::make_exception("importutil.merge-here requires 1 argument");
  }

  module *from = ref_cast<module>(argv[0]);

  if (from == nullptr) {
    throw cedar::make_exception(
        "importutil.merge-here requires a module argument");
  }


  // TODO please implement an iterator for attr_map... is
  //      an awful hack for what needs to be done here...
  for (int i = 0; i < 8; i++) {
    attr_map::bucket *b;
    for (b = from->m_attrs.m_buckets[i]; b != nullptr; b = b->next) {
      target->setattr_fast(b->key, b->val);
    }
  }

  return nullptr;
}

void bind_importutil(void) {
  module *mod = new module("importutil");

  mod->def("merge-here", merge_here);

  define_builtin_module("importutil", mod);
}
