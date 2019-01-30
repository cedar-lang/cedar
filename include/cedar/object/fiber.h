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
#include <cedar/object/indexable.h>
#include <cedar/object/lambda.h>
#include <cedar/object/sequence.h>
#include <cedar/ref.h>
#include <cedar/runes.h>
#include <cedar/vm/machine.h>
#include <stack>

namespace cedar {

  struct call_frame {
    ref fn = nullptr;
    std::shared_ptr<closure> closure = nullptr;
    long fp;
  };

  class fiber : virtual public object {
   private:
    void sync(void);

   public:
    std::stack<call_frame> calls;
    long sp = 0;
    long fp = 0;
    uint8_t *ip;

    // allow quick access to values at the top of the stack
    // (updated after a fiber sync)
    lambda *current_lambda = nullptr;
    std::weak_ptr<closure> current_closure;

    vm::machine *m_vm;

    fiber(vm::machine *);
    ~fiber(void);

    inline const char *object_type_name(void) { return "fiber"; };
    u64 hash(void);

   protected:
    cedar::runes to_string(bool human = false);
  };

}  // namespace cedar
