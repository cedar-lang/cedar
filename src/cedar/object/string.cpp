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

#include <cstdio>

#include <cedar/memory.h>
#include <cedar/object.h>
#include <cedar/object/string.h>
#include <cedar/util.hpp>

using namespace cedar;

static ref the_nil = nullptr;

cedar::string::string(void) {}
cedar::string::string(cedar::runes content) { m_content = content; }

cedar::string::~string() {}

cedar::runes string::to_string(bool human) {
  cedar::runes str;
  if (!human) {
    str += "\"";
    str += m_content;
    str += "\"";
  } else {
    auto it = m_content.begin();
    while (it != m_content.end()) {
      auto c = *it++;
      if (c == '\\' && it != m_content.end()) {
        switch (*it++) {
          case '\\':
            c = '\\';
            break;
          case 'n':
            c = '\n';
            break;
          case 't':
            c = '\t';
            break;
          case 'r':
            c = '\r';
            break;
          case '"':
            c = '"';
            break;
          case 'f':
            c = '\f';
            break;
          case '\'':
            c = '\'';
            break;
            // all other escapes
          default:
            c = '?';
            break;
        }
      }
      str += c;
    }
  }
  return str;
}

void string::set_content(cedar::runes content) { m_content = content; }

cedar::runes string::get_content(void) { return m_content; }

ref string::to_number() {
  throw cedar::make_exception("Attempt to cast string to number failed");
}

u64 string::hash(void) {
  return (std::hash<cedar::runes>()(m_content) & ~0x3) | 1;
}
