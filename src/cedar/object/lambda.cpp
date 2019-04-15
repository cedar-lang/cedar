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

#include <cedar/ref.h>
#include <cedar/memory.h>
#include <cedar/object.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/objtype.h>
#include <cedar/vm/instruction.h>

using namespace cedar;

/////////////////////////////////////////////////////

closure::closure(i32 size, closure *parent, i32 index)
    : m_size(size), m_index(index) {
  m_parent = parent;
  m_vars = std::vector<ref>(size);
}

cedar::closure::~closure(void) {}

closure *closure::clone(void) {
  auto c = new closure(m_size, m_parent, m_index);
  return c;
}

ref &cedar::closure::at(int i) {
  if (i >= m_index) {
    return m_vars[i - m_index];
  }
  return m_parent->at(i);
}

/////////////////////////////////////////////////////

cedar::lambda::lambda() {
  m_type = cedar::lambda_type;
  code = nullptr;
}

cedar::lambda::lambda(vm::bytecode *bc) {
  m_type = cedar::lambda_type;
  code_type = bytecode_type;
  code = bc;
}

// provide a wrapper around the old native calling convention
// TODO: remove this and bound_function
cedar::lambda::lambda(bound_function func) {
  m_type = cedar::lambda_type;
  code_type = function_binding_type;
  function_binding = [=] (const function_callback & args) -> void {
    call_context c;
    c.coro = args.get_fiber();
    c.mod = args.get_module();
    args.get_return() = func(args.len(), args.argv(), &c);
  };
}


cedar::lambda::lambda(native_callback func) {
  m_type = cedar::lambda_type;
  code_type = function_binding_type;
  function_binding = func;
}

cedar::lambda::~lambda() {}


void lambda::call(const function_callback & args) {
  function_binding(args);
}



u64 lambda::hash(void) {
  if (code_type == bytecode_type) {
    u64 hash = 5381;
    for (u64 i = 0; i < code->get_size(); i++) {
      hash = ((hash << 5) + hash) + code->code[i];
    }
    return hash;
  }
  return reinterpret_cast<u64>(code_type == bytecode_type ? (void *)code
                                                          : &function_binding);
}

lambda *lambda::copy(void) {
  // static std::mutex l;
  lambda *new_lambda = new lambda();
  *new_lambda = *this;
  return new_lambda;

#define COPY_FIELD(name) new_lambda->name = name
  COPY_FIELD(code_type);
  COPY_FIELD(code);
  COPY_FIELD(m_closure);
  COPY_FIELD(arg_index);
  COPY_FIELD(argc);
  COPY_FIELD(vararg);
  COPY_FIELD(function_binding);
  return new_lambda;
}

void lambda::set_args_closure(closure *c, int a_argc, ref *a_argv) {
  // code->record_call(a_argc, a_argv);
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
            " expected: ", argc, " - ", this->to_string());
      }
    } else {
      if (a_argc != argc) {
        throw cedar::make_exception(
            "invalid arg count passed to function. given: ", a_argc,
            ". ", this->to_string());
      }
    }


    // loop over the concrete list...
    for (int i = 0; i < concrete; i++) {
      c->at(i + arg_index) = a_argv[i];
    }

    if (vararg) {
      ref valist = nullptr;
      for (int i = a_argc - 1; i >= argc - 1; i--) {
        valist = new_obj<list>(a_argv[i], valist);
      }
      c->at(arg_index + argc - 1) = valist;
    }
  }
}


call_state lambda::prime(int a_argc, ref *a_argv) {
  call_state p;
  p.func = this;
  p.locals = new closure(argc, m_closure, arg_index);
  p.locals->func = this;
  set_args_closure(p.locals, a_argc, a_argv);
  return p;
}

