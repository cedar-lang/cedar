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

#include "../../src/asmjit/src/asmjit/asmjit.h"

#include <cedar/ref.h>
#include <vector>


namespace cedar {
  class lambda;
  class module;
  namespace jit {


    using namespace asmjit;
    using namespace asmjit::x86;

    using reg = X86Gp;
    using mem = X86Mem;


    lambda *compile(ref, module *);


    class compiler {
      ref base_obj;
      FileLogger logger;
      CodeHolder code;
      X86Compiler cc;

      reg cb;

      void init(void);

     public:
      explicit compiler(ref);
      explicit compiler(std::string);

      lambda *run(module*);



      reg compile_object(ref obj);
      reg compile_list(ref obj);
      reg compile_number(ref obj);
      reg compile_symbol(ref obj);
    };

  }  // namespace jit
}  // namespace cedar

#endif
