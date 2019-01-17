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
#include <cedar/ref.h>

namespace cedar {

  // any indexable object can be read from and set to randomly.
  // Example: vectors or dicts
  class indexable : public object {
   public:
    virtual ~indexable(){};
    virtual ref get(ref) = 0;
    virtual ref set(ref, ref) = 0;
    virtual ref append(ref) = 0;
    virtual i64 size(void) = 0;
  };

  inline ref idx_set(ref ir, ref k, ref v) {
    if (auto *i = ref_cast<indexable>(ir); i != nullptr) {
      return i->set(k, v);
    }

    throw cedar::make_exception("unable to index non-indexable object ", ir, " at ", k);
  }
  inline ref idx_get(ref ir, ref k) {
    if (auto *i = ref_cast<indexable>(ir); i != nullptr) {
      return i->get(k);
    }
    throw cedar::make_exception("unable to index non-indexable object ", ir, " at ", k);
  }

  inline ref idx_append(ref ir, ref v) {
    if (auto *i = ref_cast<indexable>(ir); i != nullptr) {
      return i->append(v);
    }
    throw cedar::make_exception("unable to append to non-indexable object ", ir);
  }
  inline ref idx_size(ref ir) {
    if (auto *i = ref_cast<indexable>(ir); i != nullptr) {
      return i->size();
    }
    throw cedar::make_exception("unable to get size of non-indexable object ", ir);
  }
}  // namespace cedar



