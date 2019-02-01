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

#ifndef _OBJTYPE_H
#define _OBJTYPE_H


#include <cedar/object.h>
#include <cedar/runes.h>
#include <vector>
#include <functional>

#include <cedar/ref.h>

namespace cedar {

  namespace vm {
    class machine;
  }

  // fwd declare
  class dict;

  class type {
    public:


      using method = std::function<ref(int, ref*, vm::machine*)>;


      dict *m_fields = nullptr;
      std::vector<type*> m_parents;
      cedar::runes m_name;

      method f_first;
      method f_rest;

      method f_str;
      method f_new;

      type(cedar::runes);
      ~type(void);

  };


#define FOR_EACH_OBJECT_TYPE(V) \
  V(object) \

  extern type *object_type;
  extern type *number_type;
  extern type *dict_type;
  extern type *keyword_type;
  extern type *lambda_type;
  extern type *list_type;
  extern type *vector_type;
  extern type *string_type;
  extern type *symbol_type;

  void type_init(void);
};

#endif
