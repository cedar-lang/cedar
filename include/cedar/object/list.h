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
#include <cedar/object/sequence.h>
#include <cedar/runes.h>
#include <vector>
#include <cedar/objtype.h>

namespace cedar {

  // the `list` type is the most common sequence used in cedar
  // it is what gets read by the reader when the sequence (..)
  // is encountered.
  class list : public sequence {
   public:

    ref m_first = nullptr;
    ref m_rest = nullptr;


    list(void);
    list(ref, ref);
    list(std::vector<ref> const&);
    ~list(void);
    ref first(void);
    ref rest(void);
    ref cons(ref);

    void set_first(ref);
    void set_rest(ref);


    inline const char *object_type_name(void) {
      if (m_rest.is_nil() && m_first.is_nil()) return "nil";
      return "list";
    };

    u64 hash(void);

   protected:
    inline cedar::runes to_string(bool) {
      cedar::runes s;

      if (rest().is_nil()) {
        if (first().is_nil()) return "(nil)";
        s += "(";
        s += first().to_string();
        s += ")";
        return s;
      }

      if (is_pair()) {
        s += "(";
        s += first().to_string();
        s += " . ";
        s += rest().to_string();
        s += ")";
        return s;
      }

      s += "(";
      ref curr = this;

      while (true) {
        s += curr.first().to_string();

        if (curr.rest().is_nil()) {
          break;
        } else {
          s += " ";
        }

        if (!curr.rest().is_nil()) {
          if (curr.rest()->is_pair()) {
            s += curr.rest().first().to_string();
            s += " . ";
            s += curr.rest().rest().to_string();
            break;
          }
        }

        curr = curr.rest();
      }

      s += ")";

      return s;
    }
  };

  inline ref newlist() { return nullptr; };

  template <typename F, typename... R>
  inline ref newlist(F first, R... rest) {
    ref l = new_obj<list>(nullptr, newlist(rest...));
    l.set_first(first);
    return l;
  }



  inline ref append(ref l1, ref l2) {
    if (l1.is_nil()) {
      return l2;
    }
    return new list(l1.first(), append(l1.rest(), l2));
  }
}  // namespace cedar
