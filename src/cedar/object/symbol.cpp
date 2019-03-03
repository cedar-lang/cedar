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

#include <functional>
#include <string>

#include <cedar/memory.h>
#include <cedar/object.h>
#include <cedar/object/symbol.h>
#include <cedar/objtype.h>
#include <cedar/util.hpp>
#include <flat_hash_map.hpp>
#include <mutex>
#include <vector>

using namespace cedar;


static std::mutex intern_lock;
struct symbol_table_entry {
  u64 hash = 0;
  cedar::runes sym;
};

static ska::flat_hash_map<u64, cedar::runes> symbol_table;





static u64 find_or_insert_symbol_table(cedar::runes sym) {
  intern_lock.lock();
  u64 hash = std::hash<cedar::runes>()(sym);
  symbol_table[hash] = sym;
  intern_lock.unlock();
  return hash;
}



intern_t symbol::intern(cedar::runes s) {
  return find_or_insert_symbol_table(s);
}
runes symbol::unintern(intern_t i) {
  intern_lock.lock();
  if (symbol_table.count(i) != 0) {
    cedar::runes s = symbol_table.at(i);
    intern_lock.unlock();
    return s;
  }
  intern_lock.unlock();
  throw cedar::make_exception("unable to find symbol intern id: ", i);
}

cedar::symbol::symbol(void) { m_type = symbol_type; }


cedar::symbol::symbol(cedar::runes content) {
  m_type = symbol_type;
  id = find_or_insert_symbol_table(content);
}


cedar::symbol::~symbol() {}

cedar::runes symbol::to_string(bool) { return symbol_table[id]; }

void symbol::set_content(cedar::runes content) {
  id = find_or_insert_symbol_table(content);
}

cedar::runes symbol::get_content(void) { return to_string(); }

std::vector<runes> cedar::split_dot_notation(cedar::runes sym) {
  std::vector<runes> parts;
  std::u32string delim = U".";
  size_t pos = 0;
  std::u32string s = sym;
  runes token;
  while ((pos = s.find(delim)) != std::string::npos) {
    token = s.substr(0, pos);
    parts.push_back(token);
    s.erase(0, pos + delim.length());
  }
  parts.push_back(s);
  return parts;
}

u64 symbol::hash(void) { return id; }

