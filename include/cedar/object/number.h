
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

namespace cedar {

  // number is a special case object. It isn't
  // meant to be constructed. as all numbers
  // simply live inside a reference and all
  // arithmatic is delegated to references
  // attempting to construct a number class
  // will result in an exception.
  //
  // this class simply exists to allow type checking
  class number : public object {
   public:
    enum num_type {
      unknown,
      floating,
      integer,
    };

    number(void);
    number(double d);
    number(i64 i);

    ~number(void);

    inline bool is_flt() { return ntype == num_type::floating; }
    inline bool is_int() { return ntype == num_type::integer; }


    template <typename T>
    inline T num_cast(void) {
      if (is_flt()) {
        return (T)m_flt;
      }
      if (is_int()) {
        return (T)m_int;
      }
      throw cedar::make_exception(
          "attempt to cast non-number reference to a number");
    }

    inline double to_float(void) { return num_cast<double>(); }
    inline i64 to_int(void) { return num_cast<i64>(); }


    u64 hash(void);

   protected:
    i8 ntype = num_type::unknown;
    union {
      i64 m_int;
      f64 m_flt;
    };
    cedar::runes to_string(bool human = false);
  };

}  // namespace cedar
