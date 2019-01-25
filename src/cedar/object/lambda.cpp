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

#include <cstdio>
#include <string>

#include <cedar/memory.h>
#include <cedar/object.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/vm/instruction.h>

using namespace cedar;

/////////////////////////////////////////////////////

closure::closure(i32 size, std::shared_ptr<closure> parent, i32 index)
    : m_size(size), m_index(index) {
  m_parent = parent;
  m_vars = std::vector<ref>(size);
}

cedar::closure::~closure(void) {}

std::shared_ptr<closure> closure::clone(void) {
  auto c = std::make_shared<closure>(m_size, m_parent, m_index);
  return c;
}

ref &cedar::closure::at(int i) {
  return i >= m_index ? m_vars[i - m_index] : m_parent->at(i);
}

/////////////////////////////////////////////////////

cedar::lambda::lambda() { code = std::make_shared<cedar::vm::bytecode>(); }

cedar::lambda::lambda(std::shared_ptr<vm::bytecode> bc) {
  code_type = bytecode_type;
  code = bc;
}

cedar::lambda::lambda(bound_function func) {
  code_type = function_binding_type;
  function_binding = func;
}

cedar::lambda::~lambda() {}

cedar::runes lambda::to_string(bool human) {
  cedar::runes str;
  if (defining_expr.is_nil()) {
    char addr_buf[30];
    if (code_type == bytecode_type) {
      std::sprintf(addr_buf, "%p", (void *)code.get());
    } else if (code_type == function_binding_type) {
      std::sprintf(addr_buf, "binding %p", (void *)function_binding);
    }

    str += "<lambda ";
    str += addr_buf;
    str += ">";
  } else {
    str += defining_expr.to_string(false);
  }
  return str;
}

u64 lambda::hash(void) {
  return reinterpret_cast<u64>(code_type == bytecode_type
                                   ? (void *)code.get()
                                   : (void *)function_binding);
}

lambda *lambda::copy(void) {
  lambda *new_lambda = new lambda();
  *new_lambda = *this;
  new_lambda->refcount = 0;
  return new_lambda;
#define COPY_FIELD(name) new_lambda->name = name
  COPY_FIELD(code_type);
  COPY_FIELD(code);
  COPY_FIELD(closure);
  COPY_FIELD(defining_expr);
  COPY_FIELD(arg_index);
  COPY_FIELD(argc);
  COPY_FIELD(vararg);
  COPY_FIELD(function_binding);
  return new_lambda;
}

// the prime_args function creates a closure and loads the values into it
// as would be expected for whatever calling convention this lambda has
// ie: for varargs
void lambda::prime_args(int a_argc, ref *a_argv) {
  this->closure = std::make_shared<cedar::closure>(argc, closure, arg_index);

  if (a_argc != 0 && a_argv != nullptr) {
    // here we need to setup some variables which will be derived
    // from the argument state passed in.
    // because we are passed a bare pointer (presumably to the stack)
    // we can't trust modifying values outside the realm of the arguments
    // passed in here.
    //
    // a lambda has the number of arguments that it takes up in the closure
    // but the argc passed in to this function may not match up correctly.
    // This is because of varargs and concrete args. concrete args are simply,
    // in the case of (fn (a b & c) ..), 'a' and 'b' and the vararg capture is
    // 'c'
    int concrete = argc;
    if (vararg) {
      // if vararg, there is one less argument in the concrete list
      concrete--;
      if (concrete > a_argc) {
        throw cedar::make_exception(
            "invalid arg count passed to function. given: ", a_argc,
            " expected: ", argc, " - " , this->to_string());
      }
    } else {
      if (a_argc != argc) {
        throw cedar::make_exception(
            "invalid arg count passed to function. given: ", a_argc,
            " expected: ", argc, " - " , this->to_string());
      }
    }

    // loop over the concrete list...
    for (int i = 0; i < concrete; i++) {
      closure->at(i + arg_index) = a_argv[i];
    }

    if (vararg) {
      ref valist = nullptr;
      for (int i = a_argc - 1; i >= argc - 1; i--) {
        valist = new_obj<list>(a_argv[i], valist);
      }
      closure->at(argc - 1) = valist;
    }
  }
}

