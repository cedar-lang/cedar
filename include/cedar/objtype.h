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
#include <cedar/object/lambda.h>
#include <vector>
#include <functional>

#include <cedar/ref.h>

namespace cedar {

  namespace vm {
    class machine;
  }

  class type;

  class type : public object {
    public:


      using method = std::function<ref(int, ref*, vm::machine*)>;
      std::vector<type*> m_parents;
      cedar::runes m_name;


      attr_map m_fields;
      void set_field(ref, ref);
      void set_field(cedar::runes, bound_function);
      ref get_field(ref);
      ref get_field_fast(int);


      type(cedar::runes);
      ~type(void);

      inline const char *object_type_name(void) {
        return "type";
      }

      inline u64 hash(void) {
        // types are only equal if they are the same type object in memory
        return (u64)this;
      }

  };


#define FOR_EACH_OBJECT_TYPE(V) \
  V(object) \


  extern type *type_type;
  extern type *object_type;
  extern type *list_type;

  extern type *nil_type;
  extern type *number_type;
  extern type *string_type;

  extern type *vector_type;
  extern type *dict_type;

  extern type *symbol_type;

  // initialize all the builtin types
  void type_init(void);
};

#endif
