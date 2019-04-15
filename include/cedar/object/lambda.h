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

#include <cedar/call_state.h>
#include <cedar/object.h>
#include <cedar/ref.h>
#include <cedar/runes.h>
#include <cedar/vm/binding.h>
#include <cedar/vm/bytecode.h>
#include <cedar/vm/machine.h>
#include <exception>
#include <functional>

#include <gc/gc_cpp.h>
#include <cedar/native_interface.h>

// define the number of references to store in the
// closure itself, instead of in the vector
#define CLOSURE_INTERNAL_SIZE 0



namespace cedar {

  class module;
  class fiber;



  using native_callback = std::function<void(const function_callback&)>;

  // closure represents a wraper around closed values
  // in functions, also known as "freevars" in LC
  // It will attempt to store things in a fixed sized
  // array if it can, but will also allocate a vector
  // if needed
  class closure {
   public:
    i32 m_size = 0;
    i32 m_index = 0;
    closure *m_parent;
    lambda *func = nullptr;

    std::vector<ref> m_vars;
    // constructor to allocate n vars of closure space
    //
    closure(i32, closure * = nullptr, i32 = 0);
    ~closure(void);
    closure *clone(void);
    ref &at(int);
  };



  class lambda : public object {
   public:
    enum lambda_type {
      bytecode_type,
      function_binding_type,
    };

    char code_type = bytecode_type;
    vm::bytecode *code;
    closure *m_closure = nullptr;
    module *mod = nullptr;
    ref name;
    ref self = nullptr;
    ref defining;
    i16 arg_index = 0;
    i8 argc = 0;
    bool vararg = false;
    bool macro = false;
    native_callback function_binding;




    lambda(void);
    lambda(vm::bytecode *);
    lambda(bound_function);
    lambda(native_callback);
    ~lambda(void);
    u64 hash(void);
    lambda *copy(void);

    void call(const function_callback&);

    call_state prime(int argc = 0l, ref *argv = nullptr);
    void set_args_closure(closure *, int argc = 0, ref *argv = nullptr);
  };



}  // namespace cedar
