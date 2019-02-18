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

#include <cedar/memory.h>
#include <cedar/object.h>
#include <cedar/ref.h>

#include <cedar/object/dict.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/object/nil.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <cedar/objtype.h>
#include <cedar/runes.h>
#include <cedar/vm/binding.h>
#include <cedar/vm/machine.h>

using namespace cedar;

long cedar::object_count = 0;

object *cedar::obj_root = nullptr;

// the object constructor and destructor keeps
// track of how many objects are allocated
object::object(void) {
  m_type = object_type;

  next = obj_root;

  object_count++;
}

object::~object(void) { object_count--; }




bool object::is_pair(void) {
  // a thing cannot be a pair if it isn't a list
  list *lst = this->as<list>();
  ref rest = lst->rest();
  if (rest.is_nil()) return false;
  if (rest.get_type() == list_type) return false;
  return true;
}




ref object::getattr_fast(int i) {
  static int __class__ID = get_symbol_intern_id("__class__");
  static int __addr__ID = get_symbol_intern_id("__addr__");

  if (i == __class__ID) {
    return m_type;
  }

  if (i == __addr__ID) {
    return (i64)this;
  }

  if (auto *b = m_attrs.buck(i); b != nullptr) {
    return b->val;
  }

  if (m_type != nullptr) {
    ref val = nullptr;
    if (auto *b = m_type->m_fields.buck(i); b != nullptr) {
      return b->val;
    }
    for (auto *t : m_type->m_parents) {
      if (auto *b = t->m_fields.buck(i); b != nullptr) {
        return b->val;
      }
    }

    return object_type->get_field_fast(i);
  }

  throw cedar::make_exception("attribute '", get_symbol_id_runes(i), "' on object not found");
}

attr_map::bucket *object::getattrbucket(int i) {
  return m_attrs.buck(i);
}

void object::setattr_fast(int k, ref v) { m_attrs.set(k, v); }

ref object::getattr(ref k) {
  if (symbol *s = ref_cast<symbol>(k); s != nullptr) {
    return getattr_fast(s->id);
  }
  throw cedar::make_exception("unable to getattr, '", k, "' from object. requires symbol as key");
}

void object::setattr(ref k, ref v) {
  if (symbol *s = ref_cast<symbol>(k); s != nullptr) {
    return setattr_fast(s->id, v);
  }
  throw cedar::make_exception("unable to setattr, '", k, "' from object. requires symbol as key ");
}

void object::setattr(runes name, bound_function fn) {
  ref lam = new lambda(fn);
  int id = get_symbol_intern_id(name);
  return setattr_fast(id, lam);
}

ref object::self_call(int id, bool *valid) {
  ref fn = getattr_fast(id);
  if (auto *l = ref_cast<lambda>(fn); l != nullptr) {
    ref self = this;
    ref res = vm::call_function(l, 1, &self);
    if (valid != nullptr) *valid = true;
    return res;
  }
  if (valid != nullptr) *valid = false;
  return nullptr;
}

cedar::runes object::to_string(bool human) {
  static int str_id = get_symbol_intern_id("str");
  static int repr_id = get_symbol_intern_id("repr");

  bool valid;
  ref res = self_call(human ? str_id : repr_id, &valid);
  if (valid)
    if (auto *s = ref_cast<string>(res); s != nullptr) {
      return s->get_content();
    }

  // if the above check and call did not work, return the
  // default repr of some object
  cedar::runes s;
  s += "<object of type '";
  if (m_type != nullptr) {
    s += m_type->m_name;
  } else {
    s += "Object";
  }
  s += "' at ";
  char buf[20];
  sprintf(buf, "%p", this);
  s += buf;
  s += ">";
  return s;
}

u64 object::hash(void) {
  static int hash_id = get_symbol_intern_id("hash");
  bool valid;
  ref res = self_call(hash_id, &valid);
  if (valid) return res.to_int();

  throw cedar::make_exception("Unable to hash unknown object of type ", m_type->to_string());
}

const char *object::object_type_name(void) { return "object"; };

// attr_map functions
static constexpr u8 attr_map_size = 8;

attr_map::~attr_map(void) {
  if (m_buckets != nullptr) {
    for (int i = 0; i < attr_map_size; i++) {
      bucket *b = m_buckets[i];
      while (b != nullptr) {
        bucket *n = b->next;
        delete b;
        b = n;
      }
    }

    delete[] m_buckets;
  }
}

void attr_map::init(void) {
  if (m_buckets == nullptr) {
    m_buckets = new bucket *[attr_map_size];
    for (int i = 0; i < attr_map_size; i++) {
      m_buckets[i] = nullptr;
    }
  }
}

ref attr_map::at(int i) {
  init();

  int ind = i & (attr_map_size - 1);

  bucket *b = m_buckets[ind];

  while (b != nullptr) {
    if (b->key == i) {
      return b->val;
    }
    b = b->next;
  }
  throw cedar::make_exception("attribute '", get_symbol_id_runes(i), "' not found");
};

bool attr_map::has(int i) {
  init();
  int ind = i & (attr_map_size - 1);
  bucket *b = m_buckets[ind];
  while (b != nullptr) {
    if (b->key == i) {
      return true;
    }
    b = b->next;
  }
  return false;
};

attr_map::bucket *attr_map::buck(int i) {
  init();
  int ind = i & (attr_map_size - 1);
  bucket *b = m_buckets[ind];
  while (b != nullptr) {
    if (b->key == i) {
      return b;
    }
    b = b->next;
  }
  return nullptr;
};

void attr_map::set(int k, ref v) {
  init();

  int ind = k & (attr_map_size - 1);

  bucket *b = m_buckets[ind];

  while (b != nullptr) {
    if (b->key == k) {
      b->val = v;
      return;
    }
    b = b->next;
  }

  b = new bucket();

  b->next = m_buckets[ind];
  b->key = k;
  b->val = v;

  m_buckets[ind] = b;
}
