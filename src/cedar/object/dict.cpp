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
#include <cedar/object/dict.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/ref.h>
#include <cedar/util.hpp>
#include <cedar/objtype.h>
#include <functional>
#include <string>

using namespace cedar;

cedar::dict::dict(void) {
  m_type = dict_type;
  m_buckets = std::vector<bucket *>(DICT_DEFAULT_SIZE);
}

cedar::dict::dict(int size) {
  m_type = dict_type;
  m_buckets = std::vector<bucket *>(size);
}

cedar::dict::~dict(void) {
  for (bucket *b : m_buckets) {
    while (b != nullptr) {
      bucket *n = b->next;
      delete b;
      b = n;
    }
  }
}

void dict::rehash(int new_size) {
  auto new_b = std::vector<bucket *>(new_size);
  for (bucket *b : m_buckets) {
    while (b != nullptr) {
      bucket *n = b->next;
      u64 new_ind = b->hash % new_size;
      b->next = new_b[new_ind];
      new_b[new_ind] = b;
      b = n;
    }
  }
  m_buckets = new_b;
}

dict::bucket *dict::find_bucket(ref k) {
  u64 hash = k.hash();
  int ind = hash % m_buckets.size();
  bucket *b = m_buckets[ind];

  while (b != nullptr) {
    // do a quick hash check, as it's faster than
    // a type comparison
    if (hash == b->hash) {
      // if the hash check was valid, do the slower
      // type equality check
      if (k == b->key) {
        return b;
      }
    }
    b = b->next;
  }
  return nullptr;
}

cedar::runes dict::to_string(bool human) {
  cedar::runes str = "{";

  int written = 0;
  for (bucket * start : m_buckets) {
    for (auto *b = start; b != nullptr; b = b->next) {
      if (written++ > 0) str += "  ";
      str += b->key.to_string();
      str += " ";
      str += b->val.to_string();
    }
  }
  str += "}";
  return str;
}

ref dict::get(ref k) {
  bucket *b = find_bucket(k);
  if (b == nullptr) {
    throw make_exception("unable to find key ", k, " in bucket");
  }
  return b->val;
}

bool dict::has_key(ref k) { return find_bucket(k) != nullptr; }

ref dict::set(ref k, ref v) {

  if (bucket *b = find_bucket(k); b != nullptr) {
    b->val = v;
  } else {
    m_size++;
    bucket *nb = new bucket();
    nb->hash = k.hash();
    nb->key = k;
    nb->val = v;
    int ind = nb->hash % m_buckets.size();
    nb->next = m_buckets[ind];
    m_buckets[ind] = nb;
  }

  return v;
}

ref dict::append(ref v) {
  set(v.first(), v.rest());
  return v.rest();
}

u64 dict::hash(void) {
  u64 x = 0;
  u64 y = 0;

  i64 mult = 1000003UL;  // prime multiplier

  x = 0x345678UL;

  for (bucket *b : m_buckets) {
    while (b != nullptr) {
      bucket *n = b->next;
      y = b->hash;
      x = (x ^ y) * mult;

      mult += (u64)(852520UL + 2);

      y = b->val.hash();
      x = (x ^ y) * mult;
      mult += (u64)(852520UL);
      x += 97531UL;

      b = n;
    }
  }
  return x;
}

ref dict::keys(void) {
  ref l = nullptr;
  for (bucket *b : m_buckets) {
    while (b != nullptr) {
      l = new list(new list(b->key, b->val), l);
      b = b->next;
    }
  }
  return l;
}
