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
#include <cedar/object/symbol.h>
#include <cedar/memory.h>
#include <cedar/util.hpp>
#include <cedar/objtype.h>

#include <vector>

using namespace cedar;



struct symbol_table_entry {
  u64 hash = 0;
  cedar::runes sym;
};


std::vector<symbol_table_entry> symbol_table;

cedar::runes cedar::get_symbol_id_runes(int i) {
  return symbol_table[i].sym;
}

static int find_or_insert_symbol_table(cedar::runes sym) {
  u64 hash = std::hash<cedar::runes>()(sym);
  for (size_t i = 0; i < symbol_table.size(); i++) {
    if (hash == symbol_table[i].hash) {
      return i;
    }
  }
  symbol_table.push_back({hash, sym});
  return symbol_table.size() - 1;
}


i32 cedar::get_symbol_intern_id(cedar::runes s) {
  return find_or_insert_symbol_table(s);
}

cedar::symbol::symbol(void) {
  m_type = symbol_type;
}
cedar::symbol::symbol(cedar::runes content) {
  m_type = symbol_type;
  id = find_or_insert_symbol_table(content);
}


cedar::symbol::~symbol() {
}

cedar::runes symbol::to_string(bool) {
	return symbol_table[id].sym;
}

void symbol::set_content(cedar::runes content) {
  id = find_or_insert_symbol_table(content);
}

cedar::runes symbol::get_content(void) {
	return to_string();
}



u64 symbol::hash(void) {
	return symbol_table[id].hash;
}


