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

#include <cedar/ref.h>
#include <cedar/runes.h>
#include <cedar/vm/binding.h>
#include <cedar/vm/bytecode.h>
#include <cedar/vm/compiler.h>

#include <cstdio>
#include <map>
#include <tuple>
#include <vector>

namespace cedar {

  class lambda;
  namespace vm {

    class machine {
     protected:
      ref globals;
      std::vector<ref> symbol_table;
      cedar::vm::compiler m_compiler;


      // the global table is just a vector of objects which the
      // compiler will index into in O(1) time
      std::vector<ref> global_table;
      // becuase the global_table doesn't have any name binding associated
      // with it, we must have a mapping from hash values to indexes
      std::map<u64, i64> global_symbol_lookup_table;

      friend cedar::vm::compiler;

      // uint64_t stacksize = 0;
      // ref *stack = nullptr;

     public:
      ref true_value;
      machine(void);
      ~machine(void);

      void bind(ref, ref);
      void bind(cedar::runes, bound_function);
      ref find(ref &);

      /*
       * given some object reference,
       * compile it into this specific target
       */
      ref eval(ref);
      ref eval_lambda(lambda *);
    };
  }  // namespace vm
}  // namespace cedar
