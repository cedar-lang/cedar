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

#ifndef __CDRJIT__
#define __CDRJIT__

#include <stdio.h>
#include <stdlib.h>

#include <gc/gc.h>
#include <gc/gc_cpp.h>


#define ASMJIT_CUSTOM_ALLOC GC_MALLOC
#define ASMJIT_CUSTOM_REALLOC GC_REALLOC
#define ASMJIT_CUSTOM_FREE GC_FREE

#include "../../src/asmjit/src/asmjit/asmjit.h"

#include <cedar/ast.h>
#include <cedar/native_interface.h>
#include <cedar/ref.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <sys/mman.h>


namespace cedar {
  namespace ast {
    struct node;
  };
  class lambda;
  class module;
  namespace jit {

    class compiler;

    using namespace ast;
    using namespace asmjit;
    using namespace asmjit::x86;

    using reg = X86Gp;
    using mem = X86Mem;


    /**
     * A code_handle allows the GC to cleanup jit pages after they are
     * done being used. It basically just stores a void* and has an apply()
     * method that allows the calling of that code
     */
    class code_handle : public gc_cleanup {
      ref expr;
      module *mod;
      size_t size = 0;
      std::atomic<int> calls = 0;
      std::atomic<bool> compiled;
      std::mutex compile_lock;
      void *code = nullptr;
     public:
      using jit_function_handle = void (*)(const function_callback &);
      code_handle(void* c, size_t s);
      code_handle(ref, module*);
      ~code_handle(void);

      void apply(const function_callback &cb);

    };


    lambda *compile(ref, module *);
    lambda *compile(cedar::runes, module *);




    struct stkregion {
      short ind = 0;
      bool used = false;
      short size = 0;
      reg ptr;
    };

    class compiler {
     protected:
      ref base_obj;
      FileLogger logger;
      CodeHolder code;
      X86Compiler cc;
      reg cb;
      mem stk;

      module *mod;
      // 4 byte alignment on the stack is pretty typical
      int alignment = 4;
      // a vector of length 'words' that keeps track of which
      // word of the stack is allocated or not
      std::vector<bool> regions;
      // how many 'regions' of memory to allocate onto the end of the region
      // list. Returns the index to the base region
      int sbrk(int region_count);
      void init(void);

     public:
      explicit compiler(ref);
      explicit compiler(std::string);

      // run the compiler, returning code pointer, filling in the size
      void *run(module *, size_t*);


      // top level compile function, takes a dst register and any ast::node
      void compile_node(reg dst, ast::node *obj);

      void compile_nil(reg dst);
      void compile_number(reg dst, ast::node *obj);
      void compile_symbol(reg dst, ast::node *obj);
      void compile_call(reg dst, ast::node *obj);
      void compile_if(reg dst, ast::node *obj);

      void dump_stack(void);

      stkregion salloc(short size);
      // release a stack region. Like free but at compile time and statically
      void sfree(stkregion &);
    };

  }  // namespace jit
}  // namespace cedar

#endif
