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

#include <cedar/jit.h>


using namespace cedar;
using namespace cedar::jit;


code_handle::code_handle(void *c, size_t s) {
  compiled = true;
  size = s;
  code = c;
}


code_handle::code_handle(ref e, module *m) {
  compiled = false;
  expr = e;
  mod = m;
}


void code_handle::apply(const function_callback &cb) {
  // if the object is not compiled, try to get a lock in order to compile
  // it. Then once we have the lock, check again as some other thread
  // could have done the compilation while we were waiting on it.
  if (!compiled) {
    std::unique_lock lk(compile_lock);
    if (!compiled) {
      // TODO delayed compilation
      compiler c(expr);
      code = c.run(mod, &size);
      compiled = true;
    }
  }

  // the code is now compiled for sure
  calls++;
  cb.get_return() = 1;
  return ptr_as_func<jit_function_handle>(code)(cb);
}


code_handle::~code_handle(void) {
  if (code != nullptr) {
    munmap(code, size);
    printf("code handle deleted %zu %p. %d calls\n", size, code, calls.load());
  }
}
