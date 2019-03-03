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
#include <cedar/runes.h>
#include <cedar/vm/binding.h>
#include <cedar/vm/bytecode.h>
#include <cedar/vm/machine.h>
#include <atomic>
#include <functional>
#include <memory>

// define the number of references to store in the
// closure itself, instead of in the vector
#define CLOSURE_INTERNAL_SIZE 0

namespace cedar {

  class module;

  // closure represents a wraper around closed values
  // in functions, also known as "freevars" in LC
  // It will attempt to store things in a fixed sized
  // array if it can, but will also allocate a vector
  // if needed
  class closure {
   public:
    i32 m_size = 0;
    i32 m_index = 0;
    std::shared_ptr<closure> m_parent;

    std::vector<ref> m_vars;
    // constructor to allocate n vars of closure space
    //
    closure(i32, std::shared_ptr<closure> = nullptr, i32 = 0);
    ~closure(void);
    std::shared_ptr<closure> clone(void);
    ref &at(int);
  };


  class lambda : public object {
   public:
    enum lambda_type {
      bytecode_type,
      function_binding_type,
    };

    lambda_type code_type = bytecode_type;
    vm::bytecode *code;
    std::shared_ptr<closure> m_closure = nullptr;


    module *mod = nullptr;
    ref name;
    ref self;
    ref defining;

    i32 arg_index = 0;
    i32 argc = 0;
    bool vararg = false;

    bound_function function_binding;

    lambda(void);
    lambda(cedar::vm::bytecode*);
    lambda(cedar::bound_function);
    ~lambda(void);



    u64 hash(void);
    lambda *copy(void);
    // prime_args configures the lambda with a closure and loads it
    // with the arguments according to that lambda's calling conv
    void prime_args(int argc = 0, ref *argv = nullptr);
    void set_args_closure(int argc = 0, ref *argv = nullptr);
  };
}  // namespace cedar
