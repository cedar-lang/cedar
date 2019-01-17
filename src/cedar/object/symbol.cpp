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

using namespace cedar;


// WARNING: the symbol table is interned, and therefor is not Garbage Collected or refcounted.
// This is because there is no real way to check if something is refering to a symbol or not...
std::vector<cedar::runes> cedar::symbol_table = {U"nil"};

static int find_or_insert_symbol_table(cedar::runes sym) {
  for (size_t i = 0; i < symbol_table.size(); i++) {
    if (sym == symbol_table[i]) {
      return i;
    }
  }
  symbol_table.push_back(sym);
  return symbol_table.size() - 1;
}

cedar::symbol::symbol(void) {}
cedar::symbol::symbol(cedar::runes content) {
  id = find_or_insert_symbol_table(content);
}


cedar::symbol::~symbol() {
}

cedar::runes symbol::to_string(bool) {
	return symbol_table[id];
}

void symbol::set_content(cedar::runes content) {
  id = find_or_insert_symbol_table(content);
}

cedar::runes symbol::get_content(void) {
	return to_string();
}


ref symbol::to_number() {
	throw cedar::make_exception("Attempt to cast symbol to number failed");
}



u64 symbol::hash(void) {
	return std::hash<cedar::runes>()(symbol_table[id]);
}


