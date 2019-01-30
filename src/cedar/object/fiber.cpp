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

#include <cedar/object/fiber.h>

using namespace cedar;

fiber::fiber(vm::machine *v) {
  m_vm = v;
}

fiber::~fiber(void) {}

// a hash of a fiber is just the address of the fiber
// because a fiber will never be equal to another.
u64 fiber::hash(void) {
  return (u64)this;
}

void fiber::sync(void) {
  if (calls.size() != 0) {
    current_closure = calls.top().closure;
    current_lambda = calls.top().fn.reinterpret<lambda*>();
  } else {
    current_lambda = nullptr;
  }
}


cedar::runes fiber::to_string(bool human) {

  cedar::runes str;
  str += "<fiber ";
  char addr_buf[30];
  std::sprintf(addr_buf, "%p", (void *)this);
  str += addr_buf;
  str += ">";
  return str;
}
