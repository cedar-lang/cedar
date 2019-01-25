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
#include <cedar/object/dict.h>
#include <cedar/object/indexable.h>
#include <cedar/object/sequence.h>
#include <cedar/ref.h>
#include <cedar/runes.h>
#include <cedar/vm/machine.h>
#include <vector>

namespace cedar {

  class user_type_instance;

  // user_type is an representation of a class, not an instance
  // so a user_type is meant to be instantiated in order to use values in it
  // It's not dissimilar to when you say `class Foo:` in python. That creates
  // a type called `Foo` and a global function for construction. However,
  // interacting with types or instantiating them will be through macros for
  // ease of use. mostly `(class Foo)` which creates a global class object
  // bound to `Foo`, and `(new Foo ...)` which instantiates an instance of Foo
  class user_type : public object {
   public:
    struct field {
      ref key, val;
    };

   protected:
    friend user_type_instance;
    cedar::runes m_name;
    char *m_cname;
    std::vector<field> m_fields;
    std::vector<ref> m_parents;

   public:
    user_type(runes);
    ~user_type();
    void add_parent(ref);
    void add_field(ref, ref);
    ref instantiate(int argc, ref *argv, vm::machine *);
    inline const char *object_type_name(void) { return m_cname; };
    u64 hash(void);
    runes to_string(bool human = false);
  };

  class user_type_instance : public indexable, public sequence {
   public:
    // user_type
    ref m_type = nullptr;
    // dict
    ref m_fields = nullptr;
    // user type instances should know what vm they're running on
    // so they can evaluate any lambdas or whatnot inside builtin
    // sequence operations
    vm::machine *m_vm = nullptr;

    user_type_instance(ref, ref, vm::machine *);

    ~user_type_instance(void);

    ref get(ref);
    ref set(ref, ref);
    inline ref append(ref v) { return this; }
    inline i64 size(void) { return 0; }

    cedar::runes to_string(bool human = false);

    ref first(void);
    ref rest(void);

    // these shouldn't do anything...
    // TODO: remove them in general
    inline void set_first(ref){/* NOP */};
    inline void set_rest(ref){/* NOP */};

    inline const char *object_type_name(void) {
      return m_type.object_type_name();
    };

    inline u64 hash(void) { return m_type.hash() ^ m_fields.hash(); }
  };
};  // namespace cedar
