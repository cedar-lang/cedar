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

#include <cedar/object/symbol.h>
#include <cedar/object/lambda.h>
#include <cedar/object/user_type.h>
#include <stdlib.h>
#include <string.h>

using namespace cedar;

//////////////////////////////////////////////////////////////////////

user_type::user_type(runes name) {
  m_name = name;
  std::string s = name;
  m_cname = new char[s.size() + 1];
  memset((void *)m_cname, 0, s.size() + 1);

  for (int i = 0; i < (int)s.size(); i++) {
    m_cname[i] = s[i];
  }
}

user_type::~user_type() { delete m_cname; }

void user_type::add_parent(ref p) {
  if (p != this) {
    m_parents.push_back(p);
  }
}

void user_type::add_field(ref k, ref v) {
  for (auto &f : m_fields) {
    if (f.key == k) {
      f.val = v;
      return;
    }
  }
  m_fields.push_back({.key = k, .val = v});
}

ref user_type::instantiate(int argc, ref *argv, vm::machine *m) {
  static ref new_sym = new_obj<symbol>("new");

  static ref __class__ = new_obj<symbol>("__class__");


  /*
  for (auto &p : m_parents) {
    if (user_type *put = ref_cast<user_type>(p); put != nullptr) {
      for (auto &f : put->m_fields) {
        d->set(f.key, f.val);
      }
    } else {
      throw cedar::make_exception("invalid parent type in class instantiation");
    }
  }
  for (auto &f : m_fields) {
    d->set(f.key, f.val);
  }
  */


  ref d = new dict();

  ref inst = new user_type_instance(this, d, m);
  idx_set(inst, __class__, this);

  auto * instance = ref_cast<user_type_instance>(inst);


  if (instance->has_field(new_sym)) {

    instance->get(new_sym);
    ref constructor = instance->get(new_sym);
    if (lambda *fn = ref_cast<lambda>(constructor); fn != nullptr) {
      fn = fn->copy();
      // we need to cram the `self` argument into the constructor
      // so unfortunately this requires a memory allocation then a copy
      argc++;
      ref *nargv = new ref[argc];
      nargv[0] = instance;
      for (int i = 0; i < argc - 1; i++) {
        nargv[i + 1] = argv[i];
      }

      fn->prime_args(argc, nargv);
      m->eval_lambda(fn);

      delete[] nargv;
    }
  } else {
  }

  return instance;
}

ref user_type::get_field(ref k, ref inst) {
  for (auto &f : m_fields) {
    if (f.key == k) {
      return f.val;
    }
  }

  for (auto &p : m_parents) {
    try {
      return ref_cast<user_type>(p)->get_field(k, inst);
    } catch (std::exception &e) {
      // ignore...
    }
  }
  throw cedar::make_exception("unable to find key ", k, " on ", to_string());
}

u64 user_type::hash(void) {
  u64 x = 0;
  u64 y = 0;

  i64 mult = 1000009UL;  // prime multiplier

  x = 0x12345678UL;  // :>
  for (auto & v : m_fields) {
    y = v.key.hash();
    x = (x ^ y) * mult;
    mult += (u64)(852520UL + 2);

    y = v.val.hash();
    x = (x ^ y) * mult;
    mult += (u64)(852520UL);
    x += 97531UL;
  }
  for (auto & v : m_parents) {
    y = v.hash();
    x = (x ^ y) * mult;
    mult += (u64)(852520UL);
    x += 97531UL;
  }
  return x;
}

runes user_type::to_string(bool human) {
  runes s;
  s += "<type ";
  s += m_name;
  s += " ";
  char buf[30];
  sprintf(buf, "%p", this);
  s += buf;
  s += ">";
  return s;
}

//////////////////////////////////////////////////////////////////////

user_type_instance::user_type_instance(ref type, ref fields, vm::machine *vm) {
  m_type = type;
  m_fields = fields;
  m_vm = vm;
}

user_type_instance::~user_type_instance(void) {}

ref user_type_instance::get(ref k) {
  dict *d = ref_cast<dict>(m_fields);
  user_type *t = ref_cast<user_type>(m_type);
  if (d->has_key(k)) {
    return d->get(k);
  }
  ref val = t->get_field(k, this);
  return val;
}

bool user_type_instance::has_field(ref k) {
  try {
    get(k);
    return true;
  } catch (std::exception &e) {
    return false;
  }
}

ref user_type_instance::set(ref k, ref v) { return idx_set(m_fields, k, v); }

ref user_type_instance::first(void) {
  static ref first = new_obj<symbol>("first");

  ref fnr = get(first);

  if (lambda *fn = ref_cast<lambda>(fnr); fn != nullptr) {
    fn = fn->copy();
    ref self = this;
    fn->prime_args(1, &self);
    return m_vm->eval_lambda(fn);
  }
  throw cedar::make_exception(
      "calling first on class failed because it's missing first");
  return nullptr;
}

ref user_type_instance::rest(void) {
  static ref rest = new_obj<symbol>("rest");
  ref fnr = get(rest);
  if (lambda *fn = ref_cast<lambda>(fnr); fn != nullptr) {
    fn = fn->copy();
    ref self = this;
    fn->prime_args(1, &self);
    return m_vm->eval_lambda(fn);
  }
  throw cedar::make_exception(
      "calling rest on class failed because it's missing rest");
  return nullptr;
}

runes user_type_instance::to_string(bool human) {
  static ref str = new_obj<symbol>("str");
  try {
    ref fnr = get(str);
    if (lambda *fn = ref_cast<lambda>(fnr); fn != nullptr) {
      fn = fn->copy();
      ref self = this;
      fn->prime_args(1, &self);
      return m_vm->eval_lambda(fn).to_string(true);
    }
  } catch (...) {
  }
  runes s;
  s += "<";
  s += ref_cast<user_type>(m_type)->m_name;
  s += " ";
  char buf[30];
  sprintf(buf, "%p", this);
  s += buf;
  s += ">";
  return s;
}

