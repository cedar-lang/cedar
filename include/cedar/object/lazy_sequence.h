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
#include <cedar/ref.h>
#include <cedar/object/sequence.h>
#include <cedar/vm/machine.h>

namespace cedar {

  class lazy_sequence : public sequence {
    bool evaluated = false;
    ref m_fn = nullptr;
    ref m_seq = nullptr;
    vm::machine *m_machine;

    ref seq(void);
   public:
    lazy_sequence(ref, vm::machine*);

    ref get_first(void);
    ref get_rest(void);
    void set_first(ref);
    void set_rest(ref);
    inline ref to_number() {throw cedar::make_exception("attempt to cast list to number failed");}
    inline const char *object_type_name(void) { return "list"; };
    inline u64 hash(void) {
      return seq().hash();
    }
    inline cedar::runes to_string(bool human = false) {
      return seq().to_string(human);
    }
  };
}  // namespace cedar

