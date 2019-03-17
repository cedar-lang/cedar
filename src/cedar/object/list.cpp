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

#include <cedar/object/list.h>
#include <cedar/object/nil.h>
#include <cedar/object/symbol.h>
#include <cedar/ref.h>

using namespace cedar;

list::list(void) {
  m_type = list_type;
  m_first = nullptr;
  m_rest = nullptr;
}

list::list(ref first, ref rest) {
  m_type = list_type;
  m_first = first;
  m_rest = rest;
}

list::list(std::vector<ref> const& items) {
  m_type = list_type;

  if (items.size() == 0) return;

  ref sac = new_obj<list>();
  int len = items.size();
  ref curr = nullptr;
  for (int i = len - 1; i >= 0; i--) {
    ref lst = new_obj<list>();
    ref item = items[i];
    lst.set_first(item);

    if (i - 1 >= 0) {
      if (auto *sym = ref_cast<symbol>(items[i - 1]); sym != nullptr && i != 1) {
        if (sym->get_content() == ".") {
          if (i == len - 1 && len >= 3) {
            lst.set_rest(item);
            i--;
            lst.set_first(items[i - 1]);
            i--;
            curr = lst;
            continue;
          } else {
            printf("%d\n", i);
            if (i != 0) {
              throw cedar::make_exception("illegal end of dotted list");
            }
          }
        }
      }
    }

    lst.set_rest(curr);
    curr = lst;
  }

  set_first(curr.first());
  set_rest(curr.rest());
}


list::~list(void) {
}
ref list::first(void) { return this->m_first; }

ref list::rest(void) { return this->m_rest; }


ref list::cons(ref f) {
  return new list(f, this);
}

void list::set_first(ref n_first) { this->m_first = n_first; }

void list::set_rest(ref n_rest) { this->m_rest = n_rest; }

u64 list::hash(void) {
  u64 x = 0;
  u64 y = 0;

  i64 mult = 1000003UL;  // prime multiplier

  x = 0x345678UL;

  y = m_first.hash();
  x = (x ^ y) * mult;
  mult += (u64)(852520UL + 2);

  y = m_rest.hash();
  x = (x ^ y) * mult;
  mult += (u64)(852520UL);
  x += 97531UL;
  return x;
}
