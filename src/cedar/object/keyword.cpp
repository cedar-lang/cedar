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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
#include <string>
#include <functional>
#include <cedar/object.h>
#include <cedar/objtype.h>
#include <cedar/object/keyword.h>
#include <cedar/memory.h>
#include <cedar/util.hpp>
#include <vector>

using namespace cedar;

// WARNING: the symbol table is interned, and therefor is not Garbage Collected or refcounted.
// This is because there is no real way to check if something is refering to a symbol or not...
std::vector<cedar::runes> cedar::keyword_table = {};

static int find_or_insert_symbol_table(cedar::runes sym) {
  for (size_t i = 0; i < keyword_table.size(); i++) {
    if (sym == keyword_table[i]) {
      return i;
    }
  }
  keyword_table.push_back(sym);
  return keyword_table.size() - 1;
}

cedar::keyword::keyword(void) {
  m_type = keyword_type;
}
cedar::keyword::keyword(cedar::runes content) {
  m_type = keyword_type;
  id = find_or_insert_symbol_table(content);
}


cedar::keyword::~keyword() {
}

cedar::runes keyword::to_string(bool) {
	return get_content();
}

void keyword::set_content(cedar::runes content) {
  id = find_or_insert_symbol_table(content);
}

cedar::runes keyword::get_content(void) {
	return keyword_table[id];
}


u64 keyword::hash(void) {
	return std::hash<cedar::runes>()(keyword_table[id]);
}


