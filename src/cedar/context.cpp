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

#include <cedar/context.h>
#include <cedar/memory.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/util.hpp>

#include <ctime>

#include "../lib/std.inc.h"

using namespace cedar;

context::context() {
  reader = make<cedar::reader>();
  m_evaluator = std::make_shared<cedar::vm::machine>();
}

void context::init(void) {
  std::string stdsrc;
  for (unsigned int i = 0; i < src_lib_std_inc_cdr_len; i++) {
    stdsrc.push_back(src_lib_std_inc_cdr[i]);
  }
  eval_string(cedar::runes(stdsrc));
}

ref context::eval_file(cedar::runes name) {
  cedar::runes src = cedar::util::read_file(name);
  return this->eval_string(src);
}

ref context::eval_string(cedar::runes expr) {
  parse_lock.lock();

  ref res = nullptr;
  try {
    auto top_level = reader->run(expr);
    for (auto e : top_level) {
      ref s = new_obj<symbol>("macroexpand");
      ref mac = m_evaluator->find(s);
      if (!mac.is_nil()) {
        ref wrapped = newlist(s, newlist(new_obj<symbol>("quote"), e));
        e = m_evaluator->eval(wrapped);
      }
      res = m_evaluator->eval(e);
    }
  } catch (...) {
    parse_lock.unlock();
    throw;
  }

  parse_lock.unlock();

  return res;
}
