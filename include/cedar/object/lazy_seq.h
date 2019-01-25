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

#pragma once

#include <cedar/object.h>
#include <cedar/object/list.h>
#include <cedar/object/sequence.h>
#include <cedar/ref.h>
#include <cedar/vm/machine.h>

namespace cedar {

  class lazy_seq : public sequence {
    vm::machine *m_vm;
    ref m_fn;
    bool evaluated = false;
    ref m_value;

    ref seq(void) {
      if (evaluated) return m_value;

      m_value = m_vm->eval_lambda(ref_cast<lambda>(m_fn));
      evaluated = true;
      return m_value;
    }

   public:
    inline lazy_seq(ref fn, vm::machine *vm) {
      if (ref_cast<lambda>(fn) == nullptr) {
        throw cedar::make_exception("fn argument to lazy-seq not a lambda");
      }
      m_fn = fn;
      m_vm = vm;
    }
    ~lazy_seq(){};

    inline const char *object_type_name(void) { return "lazy-seq"; };

    inline ref first(void) { return seq().first(); }
    inline ref rest(void) { return seq().rest(); }
    inline ref cons(ref f) { return new list(f, this); }

    inline void set_first(ref) {}
    inline void set_rest(ref) {}

    inline u64 hash(void) { return (u64)this; }
  };
}  // namespace cedar

