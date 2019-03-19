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

#include <cedar/object/dict.h>
#include <cedar/object/module.h>
#include <cedar/objtype.h>

#include <cedar/globals.h>
#include <cedar/object/channel.h>
#include <cedar/object/keyword.h>
#include <cedar/object/list.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <cedar/object/vector.h>
#include <cedar/vm/machine.h>

/**
 * every type in cedar is an object, so it has attributes and other things
 *
 * so every builtin type needs to be initialized
 */

using namespace cedar;

cedar::type::type(cedar::runes name) {
  m_name = name;
  if (type_type != nullptr) {
    m_type = type_type;
  }
}

cedar::type::~type(void) {}

ref type::get_field(ref k) {
  if (auto *s = ref_cast<symbol>(k); s != nullptr) {
    return get_field_fast(s->id);
  }
  throw cedar::make_exception("unable to get field '", k, "' on class ",
                              m_name);
}


ref type::get_field_fast(u64 i) {
  //
  ref val = m_fields.at(i);
  return val;
}

void type::set_field(ref k, ref val) {
  if (auto *s = ref_cast<symbol>(k); s != nullptr) {
    return m_fields.set(s->id, val);
  }
  throw cedar::make_exception("unable to set field '", k, "' on class ", m_name,
                              " to ", val);
}

void type::set_field(cedar::runes k, bound_function val) {
  auto i = symbol::intern(k);
  ref lam = new lambda(val);
  m_fields.set(i, lam);
}

////////////////////////////////////////////////////////////

static cedar_binding(type_str_lambda) {
  type *self = argv[0].stat_cast<type *>();
  cedar::runes s;
  s += "<type '";
  s += self->m_name;
  s += "' at ";
  char buf[20];
  sprintf(buf, "%p", self);
  s += buf;
  s += ">";
  return new string(s);
};

static cedar_binding(obj_str_lambda) {
  cedar::runes s;
  s += "<";
  s += argv[0].get_type()->m_name;
  s += " at ";
  char buf[20];
  sprintf(buf, "%p", argv[0].as<object>());
  s += buf;
  s += ">";
  return new string(s);
};

static bound_function check_arity(cedar::runes name, int arity,
                                  bound_function fn) {
  return [=](int argc, ref *argv, call_context *ctx) -> ref {
    if (argc != arity)
      // the exception needs to remove the first argument from the artiy
      // since it's self.
      throw cedar::make_exception("function ", name, " requires ", arity - 1,
                                  " args. given ", argc - 1);

    return fn(argc, argv, ctx);
  };
}

/**
 * every type has a very similar set of default bindings
 * ie: every type has a str method implemented the exact same
 *     same way, so this function abstracts out the setting
 *     of those attributes into a single spot
 *
 * the default function bindings are defined above this one as
 * c++ functions
 *
 * These methods are the attributes on the type, and are not
 * applied or used by instances of the type. Functions accessable
 * to instances are behind the m_fields attribute map
 */
void cedar::type_init_default_bindings(type *t) {
  t->setattr("str", type_str_lambda);
  t->setattr("repr", type_str_lambda);


  t->setattr("name", check_arity("name", 1, bind_lambda(argc, argv, machine) {
               type *self = argv[0].as<type>();
               return new string(self->m_name);
             }));

  t->setattr("add-field",
             check_arity("add-field", 3, bind_lambda(argc, argv, machine) {
               type *self = argv[0].as<type>();
               ref name = argv[1];
               ref val = argv[2];
               self->set_field(name, val);
               return val;
             }));

  t->setattr("__alloc__", bind_lambda(argc, argv, machine) {
    auto *o = new object();
    o->m_type = t;
    return o;
  });
}

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

// initialize the `type` type. The type that all types hold
type *cedar::type_type;
static void init_type_type(void) {
  // bind defaults
  type_init_default_bindings(type_type);

  type_type->setattr("__alloc__", bind_lambda(argc, argv, machine) {
    auto *nt = new type("");
    type_init_default_bindings(nt);
    return nt;
  });

  type_type->set_field(
      "set-field",
      check_arity("set-field", 3, bind_lambda(argc, argv, machine) {
        type *self = argv[0].as<type>();
        ref name = argv[1];
        ref val = argv[2];
        self->set_field(name, val);
        return nullptr;
      }));

  type_type->set_field(
      "get-field",
      check_arity("get-field", 2, bind_lambda(argc, argv, machine) {
        type *self = argv[0].as<type>();
        ref name = argv[1];
        return self->get_field(name);
      }));

  type_type->set_field(
      "add-parent",
      check_arity("add-parent", 2, bind_lambda(argc, argv, machine) {
        type *self = argv[0].as<type>();
        ref parent_ref = argv[1];
        if (!parent_ref.is<type>()) {
          throw cedar::make_exception("'add-parent' requires a type, given ",
                                      parent_ref);
        }

        self->m_parents.push_back(parent_ref.as<type>());
        return nullptr;
      }));

  type_type->set_field(
      "get-parents",
      check_arity("get-parents", 1, bind_lambda(argc, argv, machine) {
        type *self = argv[0].as<type>();
        ref v = new vector();

        for (type *t : self->m_parents) {
          v = v.cons(t);
        }

        v = v.cons(object_type);

        return v;
      }));

  type_type->set_field("new",
                       check_arity("new", 2, bind_lambda(argc, argv, machine) {
                         type *self = argv[0].as<type>();
                         ref name = argv[1];

                         if (auto *s = ref_cast<string>(name); s != nullptr) {
                           self->m_name = s->get_content();
                         } else {
                           throw cedar::make_exception(
                               "constructor to `Type` requires a string "
                               "argument as a name, given ",
                               name);
                         }

                         return nullptr;
                       }));

  def_global(new symbol("Type"), type_type);
}

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::object_type;
static void init_object_type() {
  // default fields for str and repr
  object_type->set_field("str", obj_str_lambda);
  object_type->set_field("repr", obj_str_lambda);

  object_type->setattr("__alloc__", bind_lambda(argc, argv, machine) {
    auto *o = new object();
    return o;
  });

  // bind defaults
  type_init_default_bindings(object_type);

  object_type->set_field("new",
                         bind_lambda(argc, argv, machine) { return nullptr; });

  def_global(new symbol("Object"), object_type);
}  // init_object_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//
type *cedar::list_type;
static void init_list_type() {
  // bind defaults
  type_init_default_bindings(list_type);

  list_type->set_field("first", bind_lambda(argc, argv, machine) {
    list *self = argv[0].as<list>();
    return self->first();
  });

  list_type->set_field("rest", bind_lambda(argc, argv, machine) {
    list *self = argv[0].as<list>();
    return self->rest();
  });

  list_type->set_field("new", bind_lambda(argc, argv, machine) {
    list *self = argv[0].as<list>();

    // if there are no args, this list needs to be nil instead
    if (argc == 1) {
      argv[0] = nullptr;
      return nullptr;
    }
    argv++;
    argc--;

    if (argc > 0) {
      self->m_first = argv[0];
    }

    ref l = nullptr;
    for (int i = argc - 1; i > 0; i--) {
      l = new list(argv[i], l);
    }

    self->m_rest = l;
    return nullptr;
  });

  list_type->set_field("len", bind_lambda(argc, argv, machine) {
    i64 len = 0;
    ref l = argv[0];
    while (!l.is_nil()) {
      len++;
      l = l.rest();
    }
    return len;
  });

  list_type->set_field("put",
                       check_arity("put", 2, bind_lambda(argc, argv, machine) {
                         return new list(argv[1], argv[0]);
                       }));

  list_type->set_field("peek",
                       check_arity("peek", 1, bind_lambda(argc, argv, machine) {
                         list *self = argv[0].as<list>();
                         return self->first();
                       }));

  list_type->set_field("pop",
                       check_arity("pop", 1, bind_lambda(argc, argv, machine) {
                         list *self = argv[0].as<list>();
                         return self->rest();
                       }));

  list_type->set_field("get",
                       check_arity("get", 2, bind_lambda(argc, argv, machine) {
                         i64 len = 0;
                         i64 ind = argv[1].to_int();
                         ref l = argv[0];
                         while (!l.is_nil()) {
                           if (len == ind) break;
                           len++;
                           l = l.rest();
                         }

                         return l.first();
                       }));

  list_type->setattr("__alloc__",
                     bind_lambda(argc, argv, machine) { return new list(); });

  def_global(new symbol("List"), list_type);
}  // init_list_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::nil_type;
static void init_nil_type() {
  // bind defaults
  type_init_default_bindings(nil_type);

  nil_type->setattr("__alloc__",
                    bind_lambda(argc, argv, machine) { return nullptr; });

  // constructor for Nil does nothing.
  nil_type->set_field("new",
                      bind_lambda(argc, argv, machine) { return nullptr; });

  nil_type->set_field("first",
                      bind_lambda(argc, argv, machine) { return nullptr; });

  nil_type->set_field("rest",
                      bind_lambda(argc, argv, machine) { return nullptr; });

  nil_type->set_field("len", bind_lambda(argc, argv, machine) { return 0; });

  def_global(new symbol("Nil"), nil_type);
}  // init_nil_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::number_type;
static void init_number_type() {
  // bind defaults
  type_init_default_bindings(number_type);

  number_type->setattr("__alloc__",
                       bind_lambda(argc, argv, machine) { return 0; });

  number_type->set_field("new", bind_lambda(argc, argv, machine) {
    if (argc == 2) {
      argv[0] = argv[1];
    }
    return nullptr;
  });

  number_type->set_field("reciprocal", bind_lambda(argc, argv, machine) {
    return 1.0 / argv[0].to_float();
  });

  def_global(new symbol("Number"), number_type);
}  // init_number_type
//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::string_type;
static void init_string_type() {
  // bind defaults
  type_init_default_bindings(string_type);

  string_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new string(); });

  string_type->set_field("new", bind_lambda(argc, argv, machine) {
    cedar::runes str;
    for (int i = 1; i < argc; i++) {
      str += argv[i].to_string(true);
    }
    argv[0].as<string>()->set_content(str);
    return nullptr;
  });

  string_type->set_field("first", bind_lambda(argc, argv, machine) {
    string *self = argv[0].as<string>();
    return self->first();
  });

  string_type->set_field("rest", bind_lambda(argc, argv, machine) {
    string *self = argv[0].as<string>();
    return self->rest();
  });

  string_type->set_field("len", bind_lambda(argc, argv, machine) {
    string *self = argv[0].as<string>();
    return (i64)self->get_content().size();
  });

  string_type->set_field(
      "get", check_arity("get", 2, bind_lambda(argc, argv, machine) {
        string *self = argv[0].as<string>();
        return self->get(argv[1]);
      }));

  string_type->set_field(
      "set", check_arity("set", 3, bind_lambda(argc, argv, machine) {
        string *self = argv[0].as<string>();
        return self->set(argv[1], argv[2]);
      }));
  def_global(new symbol("String"), string_type);
}  // init_string_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::vector_type;
static void init_vector_type() {
  // bind defaults
  type_init_default_bindings(vector_type);

  vector_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new vector(); });

  vector_type->set_field("new", bind_lambda(argc, argv, machine) {
    for (int i = 1; i < argc; i++) {
      argv[0] = self_call(argv[0], "put", argv[i]);
    }
    return nullptr;
  });

  vector_type->set_field("first", bind_lambda(argc, argv, machine) {
    vector *self = argv[0].as<vector>();
    return self->first();
  });

  vector_type->set_field("rest", bind_lambda(argc, argv, machine) {
    vector *self = argv[0].as<vector>();
    return self->rest();
  });

  vector_type->set_field("len", bind_lambda(argc, argv, machine) {
    vector *self = argv[0].as<vector>();
    return self->size();
  });

  vector_type->set_field(
      "get", check_arity("get", 2, bind_lambda(argc, argv, machine) {
        vector *self = argv[0].as<vector>();
        return self->get(argv[1]);
      }));

  vector_type->set_field(
      "set", check_arity("set", 3, bind_lambda(argc, argv, machine) {
        vector *self = argv[0].as<vector>();
        return self->set(argv[1], argv[2]);
      }));



  vector_type->set_field(
      "put", check_arity("put", 2, bind_lambda(argc, argv, machine) {
        vector *self = argv[0].as<vector>();
        return self->append(argv[1]);
      }));

  vector_type->set_field(
      "pop", check_arity("pop", 1, bind_lambda(argc, argv, machine) {
        vector *self = argv[0].as<vector>();
        return new vector(self->items.erase(self->items.size() - 1));
      }));

  vector_type->set_field(
      "peek", check_arity("peek", 1, bind_lambda(argc, argv, machine) {
        vector *self = argv[0].as<vector>();
        return self->items[self->items.size() - 1];
      }));


  def_global(new symbol("Vector"), vector_type);
}  // init_vector_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::dict_type;
static void init_dict_type() {
  // bind defaults
  type_init_default_bindings(dict_type);

  dict_type->setattr("__alloc__",
                     bind_lambda(argc, argv, machine) { return new dict(); });

  dict_type->set_field("new", bind_lambda(argc, argv, machine) {
    auto *self = argv[0].as<dict>();
    for (int i = 1; i < argc; i += 2) {
      idx_set(self, argv[i], argv[i + 1]);
    }
    return nullptr;
  });

  dict_type->set_field("get",
                       check_arity("get", 2, bind_lambda(argc, argv, machine) {
                         dict *self = argv[0].as<dict>();
                         return self->get(argv[1]);
                       }));

  dict_type->set_field("set",
                       check_arity("set", 3, bind_lambda(argc, argv, machine) {
                         dict *self = argv[0].as<dict>();
                         return self->set(argv[1], argv[2]);
                       }));

  def_global(new symbol("Dict"), dict_type);
}  // init_dict_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::symbol_type;
static void init_symbol_type() {
  // bind defaults
  type_init_default_bindings(symbol_type);

  symbol_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new symbol(); });

  symbol_type->set_field(
      "new", check_arity("new", 2, bind_lambda(argc, argv, machine) {
        auto *self = argv[0].as<symbol>();
        cedar::runes s = argv[1].to_string(true);
        self->set_content(s);
        return nullptr;
      }));

  def_global(new symbol("Symbol"), symbol_type);
}  // init_symbol_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::keyword_type;
static void init_keyword_type() {
  // bind defaults
  type_init_default_bindings(keyword_type);

  keyword_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new keyword(); });

  keyword_type->set_field(
      "new", check_arity("new", 2, bind_lambda(argc, argv, machine) {
        auto *self = argv[0].as<keyword>();
        cedar::runes s = argv[1].to_string(true);
        self->set_content(s);
        return nullptr;
      }));


  def_global(new symbol("Keyword"), keyword_type);
}  // init_keyword_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::lambda_type;
static void init_lambda_type() {
  // bind defaults
  type_init_default_bindings(lambda_type);

  lambda_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new lambda(); });

  lambda_type->set_field("new", bind_lambda(argc, argv, machine) {
    throw cedar::make_exception("explicit construction of lambda undefined");
    return nullptr;
  });



  auto lambda_str_bind = bind_lambda(argc, argv, ctx) {
    lambda *self = argv[0].as<lambda>();

    cedar::runes c;

    c += "<fn ";


    if (self->name.is_nil()) {
      c += "anonymous ";
    } else {
      c += self->name.to_string(false);
      c += " ";
    }

    c += "{";

    if (self->code_type == lambda::function_binding_type) {
      c += ":native true, ";
      char pbuf[20];
      sprintf(pbuf, ":addr %p,", (void *)&self->function_binding);
    }



    if (!self->self.is_nil()) {
      c += ":bound-to ";
      c += ref(self->self.get_type()).to_string(false);
      c += ", ";
    }

    c += "}";
    c += ">";
    return new string(c);
  };

  lambda_type->set_field("str", lambda_str_bind);
  lambda_type->set_field("repr", lambda_str_bind);

  def_global("Lambda", lambda_type);
}  // init_lambda_type


//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::fiber_type;
static void init_fiber_type() {
  // bind defaults
  type_init_default_bindings(fiber_type);

  fiber_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new object(); });

  fiber_type->set_field("new",
                        bind_lambda(argc, argv, machine) { return nullptr; });

  def_global(new symbol("Fiber"), fiber_type);
}  // init_fiber_type

//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::module_type;
static void init_module_type() {
  // bind defaults
  type_init_default_bindings(module_type);

  module_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new module(); });
  module_type->set_field("new",
                         bind_lambda(argc, argv, machine) { return nullptr; });
  def_global(new symbol("Module"), module_type);
}  // init_module_type


//
//
//
/////////////////////////////////////////////////////////////
//
//
//

type *cedar::channel_type;
static void init_channel_type() {
  // bind defaults
  type_init_default_bindings(module_type);

  channel_type->setattr(
      "__alloc__", bind_lambda(argc, argv, machine) { return new channel(0); });

  channel_type->set_field("new", bind_lambda(argc, argv, machine) {
    channel *self = ref_cast<channel>(argv[0]);
    if (argc == 2) {
      auto size = argv[1].to_int();
      self->set_size(size);
    }
    return nullptr;
  });
  def_global(new symbol("chan"), channel_type);

}  // init_channel_type




void cedar::type_init(void) {
  // allocate all the builtin type names and variables
#define BUILTIN_TYPE(name, str) \
  name##_type = new type(str);  \
  name##_type->m_type = type_type;

#include <cedar/builtin_types.h>
#undef BUILTIN_TYPE

// call all of the initialize functions for each type
#define BUILTIN_TYPE(name, str) init_##name##_type();
#include <cedar/builtin_types.h>
#undef BUILTIN_TYPE
}
