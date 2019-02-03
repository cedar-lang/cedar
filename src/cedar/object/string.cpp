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
#include <cedar/objtype.h>
#include <cedar/util.hpp>
#include <vector>


using namespace cedar;

static ref the_nil = nullptr;

cedar::string::string(void) {
  m_type = string_type;
}
cedar::string::string(cedar::runes content) {
  m_type = string_type;
  m_content = content;
}

cedar::string::~string() {}


struct string_char_conversion {
  char c;
  runes r;
};

cedar::runes string::to_string(bool human) {


  static std::vector<string_char_conversion> mappings = {
    {'"', "\\\""},
    {'\b', "\\b"},
    {'\n', "\\n"},
    {'\t', "\\t"},
    {'\r', "\\r"},
    {'\\', "\\"},
  };


  auto get_char_runes = [&] (char c) -> runes {
    runes str;
    for (auto & m : mappings) {
      if (m.c == c) return m.r;
    }
    str += c;
    return str;
  };

  cedar::runes str;
  if (!human) {
    str += "\"";
    for (auto & c : m_content) {
      str += get_char_runes(c);
    }
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
          case 'b':
            c = '\b';
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

u64 string::hash(void) {
  return (std::hash<cedar::runes>()(m_content) & ~0x3) | 1;
}

ref string::first(void) {
  cedar::runes str;
  str += m_content[0];
  return new string(str);
}

ref string::rest(void) {
  if (m_content.size() <= 1) return nullptr;
  cedar::runes str;
  for (u32 i = 1; i < m_content.size(); i++) str += m_content[i];
  return new string(str);
}


ref string::get(ref ind) {
  if (ind.is_number()) {
    i64 i = ind.to_int();
    if (i >= m_content.size() || i < 0) return nullptr;
    cedar::runes c;
    c += m_content[i];
    return new string(c);
  } else {
    throw cedar::make_exception("unable to index into string by a non-number");
  }
}

ref string::set(ref ind, ref val) {
  if (ind.is_number()) {
    i64 n = ind.to_int();
    if (n >= m_content.size() || n < 0) return this;

    cedar::runes c;
    for (u32 i = 0; i < m_content.size(); i++) {
      if (i == n) c += val.to_string(true);
      c += m_content[i];
    }
    return new string(c);
  } else {
    throw cedar::make_exception("unable to index into string by a non-number");
  }
}

ref string::append(ref val) {
  cedar::runes v = m_content;
  v += val.to_string(true);
  return new string(v);
}



