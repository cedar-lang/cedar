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

#include <cedar/object/lambda.h>
#include <cedar/object/lazy_sequence.h>
#include <cedar/object/list.h>
#include <cedar/object/nil.h>
#include <cedar/object/symbol.h>
#include <cedar/ref.h>


using namespace cedar;

lazy_sequence::lazy_sequence(ref f, vm::machine *m) {
  if (!f.is<lambda>()) {
    throw cedar::make_exception("lazy_sequence constructor requires a lambda reference");
  }
  m_machine = m;
  m_fn = f;
}

ref lazy_sequence::seq(void) {
  if (evaluated) return m_seq;
  if (lambda *fnt = ref_cast<lambda>(m_fn); fnt != nullptr) {
    lambda *fn = fnt->copy();
    fn->closure = std::make_shared<closure>(fn->argc, fn->closure, fn->arg_index);
    ref res = m_machine->eval_lambda(fn);
    evaluated = true;
    m_seq = res;
    return m_seq;
  }
  throw cedar::make_exception("lazy_sequence lambda value not a lambda");
}

ref lazy_sequence::get_first(void) {
  return seq().get_first();
}

ref lazy_sequence::get_rest(void) {
  return seq().get_rest();
}

void lazy_sequence::set_rest(ref) {
}
void lazy_sequence::set_first(ref) {
}

