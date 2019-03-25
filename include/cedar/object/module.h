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
#include <cedar/object/lambda.h>
#include <cedar/object/symbol.h>
#include <flat_hash_map.hpp>

namespace cedar {


  class module : public object {
   private:
    enum binding_type { PRIVATE, PUBLIC };
    struct binding {
      binding_type type = PRIVATE;
      ref val;
    };

    std::mutex lock;

    ska::flat_hash_map<intern_t, binding> m_fields;

   public:
    module(void);
    module(std::string);
    ~module(void);

    void def(std::string, ref);
    void def(std::string, bound_function);
    void def(std::string, native_callback);

    // set a private module field. Will only be accessed via a `find` from
    // within the same module. The way private is inforced is by making getattr
    // not resolve to private fields. Now there is a problem where public fields
    // can be shadowed by private ones, but I will work around that later... :)
    void set_private(intern_t, ref);

    void import_into(module *);


    ref find(intern_t, bool *, module *from = nullptr);
    virtual ref getattr_fast(u64);
    virtual void setattr_fast(u64, ref);
  };

}  // namespace cedar
