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

#ifndef _NATIVE_INTERFACE
#define _NATIVE_INTERFACE


#include <cedar/ref.h>
#include <stdlib.h>

namespace cedar {

  class fiber;
  class module;

  class function_callback {
   public:
    size_t m_argc = 0;
    ref *m_argv = nullptr;
    ref m_self = nullptr;
    fiber *m_fiber = nullptr;
    module *m_mod = nullptr;
    ref thrown = nullptr;

    ref return_value;


    inline void _do_throw_obj(ref obj) {
      threw = true;
      thrown = obj;
    }

   public:
    bool threw = false;
    inline function_callback(ref self, int argc, ref *argv, fiber *f,
                             module *mod) {
      m_self = self;
      m_argc = argc;
      m_argv = argv;
      m_fiber = f;
      m_mod = mod;
    }
    // index into the argument list
    inline ref &operator[](size_t ind) const {
      if (ind > m_argc) {
        throw std::out_of_range("invalid number of args");
      }
      return m_argv[ind];
    }

    inline ref get_thrown() { return thrown; }

    inline size_t len(void) const { return m_argc; }
    inline ref self(void) const { return m_self; }
    inline fiber *get_fiber(void) const { return m_fiber; }
    inline module *get_module(void) const { return m_mod; }

    // hacky... but it works
    inline void throw_obj(ref obj) const {
      const_cast<function_callback *>(this)->_do_throw_obj(obj);
    }
    inline ref &get_return(void) const {
      return const_cast<ref &>(return_value);
    }
    inline ref *argv(void) const { return m_argv; }
  };


  using native_callback = std::function<void(const function_callback &)>;
}  // namespace cedar

#endif
