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
#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/lambda.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/objtype.h>

using namespace cedar;

module::module(void) {
  def("*name*", new string("unnamed"));
  m_type = module_type;
}

module::module(std::string name) {
  def("*name*", new string(name));
  m_type = module_type;
}

module::~module(void) {}


void module::def(std::string name, ref val) {
  setattr_fast(symbol::intern(name), val);
}


void module::def(std::string name, bound_function val) {
  u64 id = symbol::intern(name);
  lambda *func = new lambda(val);
  func->name = new symbol(name);
  setattr_fast(id, func);
}

void module::def(std::string name, native_callback val) {
  u64 id = symbol::intern(name);
  lambda *func = new lambda(val);
  func->name = new symbol(name);
  setattr_fast(id, func);
}


void module::import_into(module *other) {
  for (auto &kv : m_fields) {
    if (kv.second.type == PUBLIC) other->m_fields[kv.first] = kv.second;
  }
}


void module::set_private(intern_t i, ref v) {
  binding b;
  b.type = PRIVATE;
  b.val = v;
  m_fields[i] = b;
}


ref module::find(u64 id, bool *valid, module *from) {
  if (m_fields.count(id) == 1) {
    binding b = m_fields.at(id);
    // if the binding is private, it should only return if from is this
    // if the binding is public, return it's val
    if ((b.type == PRIVATE && from == this) || b.type == PUBLIC) {
      if (valid != nullptr) *valid = true;
      return b.val;
    }
  }

  // if this module isn't the core, look in there for the symbol
  if (this != core_mod && core_mod != nullptr) {
    return core_mod->find(id, valid, from);
  }

  if (valid != nullptr) *valid = false;
  return nullptr;
}


static std::mutex write_lock;


#define FILE_NAME_SIZE 100

static int file_name(char *name, module *m) {
  try {
    static auto name_id = symbol::intern("*name*");

    bool found;
    ref namev = m->find(name_id, &found, nullptr);
    if (!found) return -1;

    std::string sname = namev.to_string(true);
    snprintf(name, FILE_NAME_SIZE, "%s.mod", sname.c_str());
    for (int i = 0; i < 100; i++) {
      if (name[i] == '/') name[i] = '@';
    }
    if (name[0] == '.') {
      name[0] = '@';
    }
    return 0;
  } catch (...) {
    return -1;
  }
  return -1;
}


static void store(module *m) {
  char name[100];
  if (file_name(name, m) == 0) {
    FILE *fp = fopen(name, "wb");
    serializer s(fp);
    for (auto &i : m->m_fields) {
      cedar::runes line;
      symbol sy;
      sy.id = i.first;
      try {
        s.write(ref(&sy));
        s.write(i.second.val);
      } catch (...) {
      }
    }
    fclose(fp);
  }
}

static void load(module *m) {
  try {
    char name[100];
    if (file_name(name, m) == 0) {
      FILE *fp = fopen(name, "rb");
      serializer s(fp);
      while (!feof(fp)) {
        cedar::runes line;
        ref k = s.read();
        ref v = s.read();
        std::cout << k << " = " << v << std::endl;
      }
      fclose(fp);
    }
  } catch (...) {
  }
}


ref module::getattr_fast(u64 k) {
  bool found = false;
  ref v = find(k, &found, nullptr);
  if (found) return v;
  return object::getattr_fast(k);
}




void module::setattr_fast(u64 k, ref v) {
  binding b;
  b.type = PUBLIC;
  b.val = v;
  if (m_fields.count(k) != 0) {
    // std::cout << "REASSIGN " << symbol::unintern(k) << std::endl;
  }
  m_fields[k] = b;
}

