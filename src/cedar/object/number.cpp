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

#include <cedar/object.h>
#include <cedar/object/number.h>
#include <cedar/runes.h>
#include <cedar/objtype.h>
#include <string>

using namespace cedar;


number::number(void) {
  m_type = number_type;
  number((i64)0);
}
number::number(double d) {
  m_type = number_type;
  ntype = num_type::floating;
  m_flt = d;
}
number::number(i64 i) {
  m_type = number_type;
  ntype = num_type::floating;
  m_int = i;
}

number::~number(void) {}




cedar::runes number::to_string(bool) {
  if (is_int()) {
    return std::to_string(m_int);
  }
  if (is_flt()) {
    auto str = std::to_string(m_flt);
    long len = str.length();
    for (int i = len - 1; i > 0; i--) {
      if (str[i] == '0') {
        str.pop_back();
      } else if (str[i] == '.') {
        str.pop_back();
        str += ".0";
        break;
      } else {
        break;
      }
    }
    return str;
  }
  return "0";
}



u64 number::hash(void) { return 0lu; }



