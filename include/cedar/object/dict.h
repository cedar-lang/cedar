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

#pragma once

#include <cedar/object.h>
#include <cedar/object/indexable.h>
#include <cedar/object/sequence.h>
#include <cedar/ref.h>
#include <cedar/runes.h>
#include <unordered_map>
#include <immer/map.hpp>

#include <vector>

#define DICT_DEFAULT_SIZE 10
namespace cedar {

  class dict : public object {
   private:
    struct bucket {
      u64 hash = 0;
      ref key = nullptr;
      ref val = nullptr;
      bucket *next = nullptr;
    };

    int m_size = 0;
    std::vector<bucket *> m_buckets;

    void rehash(int);

    // find a bucket given some key
    bucket *find_bucket(ref);

   public:
    dict(void);
    dict(int);
    ~dict(void);

    inline const char *object_type_name(void) { return "dict"; };
    bool has_key(ref);
    u64 hash(void);
    ref get(ref);
    ref set(ref, ref);
    ref keys(void);
    ref append(ref);
    inline i64 size(void) { return m_size; }

   protected:
    cedar::runes to_string(bool human = false);
  };

}  // namespace cedar
