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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cedar/object.h>
#include <cedar/object/coroutine.h>

using namespace cedar;

coroutine::coroutine(void) {
  stack.reserve(32);
  sp = 0;
  fp = 0;
  ip = nullptr;
}


i64 coroutine::push(ref val) {
  if (sp >= stack.size()) {
    stack.reserve(stack.size() + 32);
  }
  stack[sp] = val;
  // return the stack location that it was written to
  return sp++;
}

ref coroutine::pop(void) {
  ref val = stack[--sp];
  // dec refcount of the item on the top of the stack
  stack[sp+1] = nullptr;
  return val;
}


void coroutine::push_frame(ref) {
}

void coroutine::pop_frame(void) {

}

u64 coroutine::hash(void) {
  return (u64)this;
}

cedar::runes coroutine::to_string(bool) {
  char addr_buf[30];
  std::sprintf(addr_buf, "%p", this);
  cedar::runes str;
  str += "<coroutine ";
  str += addr_buf;
  str += ">";
  return str;
}

lambda *coroutine::program() {
  return stack[fp].reinterpret<lambda*>();
}

ref coroutine::to_number() {
	throw cedar::make_exception("Attempt to cast coroutine to number failed");
}
