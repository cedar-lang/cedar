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

// this file includes most of the standard bindings and runtime functions
// required by the cedar standard library. They will be linked at runtime via a
// dlopen and dlsym binding defined in src/main.cpp

#include <cedar.h>
#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/bytes.h>
#include <cedar/thread.h>
#include <cedar/vm/binding.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex>
#include <thread>

#include <gc/gc.h>




using namespace cedar;


ref true_value = nullptr;

#define ERROR_IF_ARGS_PASSED_IS(name, op, c)                            \
  if (argc op c)                                                        \
    throw cedar::make_exception("(" #name " ...) requires ", c, " arg", \
                                (c == 1 ? "" : "s"), "given ", argc);

cedar_binding(cedar_hash) {
  ERROR_IF_ARGS_PASSED_IS("cedar/hash", !=, 1);
  return ref((i64)argv[0].hash());
}
cedar_binding(cedar_id) {
  ERROR_IF_ARGS_PASSED_IS("cedar/id", !=, 1);

  return ref(argv[0].reinterpret<i64>());
}

cedar_binding(cedar_is) {
  ERROR_IF_ARGS_PASSED_IS("is", !=, 2);
  if (argv[0].is_obj() && argv[1].is_obj())
    return argv[0].reinterpret<u64>() == argv[0].reinterpret<u64>() ? true_value
                                                                    : nullptr;
  return argv[0] == argv[0] ? true_value : nullptr;
}

cedar_binding(cedar_add) {
  if (argc == 0) return 0;
  ref accumulator = argv[0];
  for (int i = 1; i < argc; i++) {
    accumulator = accumulator + argv[i];
  }
  return accumulator;
}

cedar_binding(cedar_sub) {
  if (argc == 1) {
    return ref{-1} * argv[0];
  }
  if (argc == 0) {
    return 0;
  }
  ref acc = argv[0];
  for (int i = 1; i < argc; i++) {
    acc = acc - argv[i];
  }
  return acc;
}

cedar_binding(cedar_mul) {
  if (argc == 0) return 1;
  ref a = argv[0];
  for (int i = 1; i < argc; i++) {
    a = a * argv[i];
  }
  return a;
}

cedar_binding(cedar_div) {
  if (argc == 1) {
    return self_call(argv[0], "reciprocal");
  }
  ref acc = argv[0];
  for (int i = 1; i < argc; i++) {
    acc = acc / argv[i];
  }
  return acc;
}

cedar_binding(cedar_mod) {
  ERROR_IF_ARGS_PASSED_IS("mod", !=, 2);
  auto a = argv[0];
  auto b = argv[1];
  if (a.is_flt() || b.is_flt()) {
    return fmod(a.to_float(), b.to_float());
  }
  return a.to_int() % b.to_int();
}


cedar_binding(cedar_pow) {
  ERROR_IF_ARGS_PASSED_IS("**", !=, 2);
  auto a = argv[0];
  auto b = argv[1];

  if (a.get_type() != number_type || b.get_type() != number_type) {
    throw cedar::make_exception("** (pow) requires two numbers, given ", a,
                                " and ", b);
  }
  return pow(a.to_float(), b.to_float());
}


cedar_binding(cedar_equal) {
  static ref fls = new symbol("false");
  if (argc < 2)
    throw cedar::make_exception(
        "(= ...) requires at least two arguments, given ", argc);
  ref first = argv[0];
  for (int i = 1; i < argc; i++) {
    if (argv[i] != first) return fls;
  }
  return true_value;
}

cedar_binding(cedar_lt) { return argv[0] < argv[1] ? true_value : nullptr; }
cedar_binding(cedar_lte) { return argv[0] <= argv[1] ? true_value : nullptr; }
cedar_binding(cedar_gt) { return argv[0] > argv[1] ? true_value : nullptr; }
cedar_binding(cedar_gte) { return argv[0] >= argv[1] ? true_value : nullptr; }

cedar_binding(cedar_print) {
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i].to_string(true);
    if (i < argc - 1) std::cout << " ";
  }
  return 0;
}
cedar_binding(cedar_println) {
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i].to_string(true);
    if (i < argc - 1) std::cout << " ";
  }
  std::cout << std::endl;
  return 0;
}


cedar_binding(cedar_cons) {
  if (argc != 2)
    throw cedar::make_exception("(cons ...) requires two argument, given ",
                                argc);

  return new list(argv[0], argv[1]);
}

cedar_binding(cedar_newlist) {
  ref l = new_obj<list>();
  ref base = l;
  for (int i = 0; i < argc; i++) {
    l.set_first(argv[i]);
    if (i < argc - 1) {
      l.set_rest(new_obj<list>());
      l = l.rest();
    }
  }

  return base;
}

cedar_binding(cedar_newdict) {
  if ((argc & 1) == 1)
    throw cedar::make_exception(
        "(dict ...) requires an even number of arguments, given ", argc);

  ref d = cedar::new_obj<cedar::dict>();
  for (int i = 0; i < argc; i += 2) {
    idx_set(d, argv[i], argv[i + 1]);
  }
  return d;
}

cedar_binding(cedar_get) {
  if (argc < 2)
    throw cedar::make_exception(
        "(get coll field [default]) requires two or three arguments, given ",
        argc);
  ref d = argv[0];
  ref k = argv[1];
  try {
    return self_call(d, "get", k);
  } catch (...) {
    if (argc == 3) return argv[2];
    throw cedar::make_exception("collection has no value at index, '", k, "'");
  }
}

cedar_binding(cedar_set) {
  if (argc != 3)
    throw cedar::make_exception("(set ...) requires two arguments, given ",
                                argc);
  // set the 0th argument's dict entry at the 1st argument to the second
  // argument
  return idx_set(argv[0], argv[1], argv[2]);
}

cedar_binding(cedar_dict_keys) {
  if (argc != 1)
    throw cedar::make_exception("(keys ...) requires one argument, given ",
                                argc);
  ref d = argv[0];
  if (auto *dp = ref_cast<dict>(d); dp != nullptr) {
    return dp->keys();
  }
  throw cedar::make_exception("(keys) call failed on non-dict");
}

cedar_binding(cedar_size) {
  if (argc != 1)
    throw cedar::make_exception("(size ...) requires one arguments, given ",
                                argc);
  return idx_size(argv[0]);
}



cedar_binding(cedar_objcount) { return cedar::object_count; }

// give a macro expansion for each open option argument
#define FOREACH_OPEN_OPTION(V) \
  V(O_RDONLY)                  \
  V(O_WRONLY)                  \
  V(O_RDWR)                    \
  V(O_NONBLOCK)                \
  V(O_APPEND)                  \
  V(O_CREAT)                   \
  V(O_TRUNC)                   \
  V(O_EXCL)

cedar_binding(cedar_os_open) {
  ERROR_IF_ARGS_PASSED_IS("os-open", !=, 2);
  ref path = argv[0];
  ref opts = argv[1];
  if (!path.is<string>() || !opts.is<number>()) {
    throw cedar::make_exception(
        "os-open requires types (os-open :string :number)");
  }
  int flags = opts.to_int();
  std::string path_s = path.to_string(true);
  return (i64)open(path_s.c_str(), flags, 0666);
}


static ref obj_dir(int argc, ref *argv, call_context *ctx) {
  if (argc != 1) {
    throw cedar::make_exception("dir requires 1 argument");
  }

  object *from = ref_cast<object>(argv[0]);

  if (from == nullptr) {
    return new vector();
  }

  ref v = new vector();

  if (from->m_attrs.m_buckets == nullptr) {
    printf("here\n");
    return v;
  }

  // TODO please implement an iterator for attr_map... is
  //      an awful hack for what needs to be done here...
  for (int i = 0; i < 8; i++) {
    attr_map::bucket *b;
    for (b = from->m_attrs.m_buckets[i]; b != nullptr; b = b->next) {
      symbol *s = new symbol();
      s->id = b->key;
      v = self_call(v, "put", s);
    }
  }

  return v;
}



cedar_binding(cedar_os_write) {
  ERROR_IF_ARGS_PASSED_IS("os-write", !=, 2);
  std::string buf = argv[1].to_string(true);
  return (i64)write(argv[0].to_int(), buf.c_str(), buf.size());
}

cedar_binding(cedar_os_read) {
  ERROR_IF_ARGS_PASSED_IS("os-read", !=, 2);
  int fd = argv[0].to_int();
  int ct = argv[1].to_int();
  char *buf = new char[ct + 1];
  buf[ct] = 0;
  read(fd, buf, ct);
  cedar::runes res = buf;
  return new string(res);
}

cedar_binding(cedar_os_close) {
  ERROR_IF_ARGS_PASSED_IS("os-close", !=, 1);
  return (i64)close(argv[0].to_int());
}

cedar_binding(cedar_os_chmod) {
  ERROR_IF_ARGS_PASSED_IS("os-chmod", !=, 2);

  if (argv[0].is<string>()) {
    std::string buf = argv[0].to_string(true);
    return (i64)chmod(buf.c_str(), argv[1].to_int());
  }
  return (i64)fchmod(argv[0].to_int(), argv[1].to_int());
}



cedar_binding(cedar_newvector) {
  immer::flex_vector<ref> vec;
  for (int i = 0; i < argc; i++) vec = vec.push_back(argv[i]);
  return new vector(vec);
}

cedar_binding(cedar_keyword) {
  ERROR_IF_ARGS_PASSED_IS("cedar/keyword", !=, 1);
  if (!argv[0].is<string>())
    throw cedar::make_exception("cedar/keyword requires a :string");
  cedar::runes s = ":";
  s += argv[0].to_string(true);
  return new_obj<keyword>(s);
}
cedar_binding(cedar_symbol) {
  ERROR_IF_ARGS_PASSED_IS("cedar/symbol", !=, 1);
  if (!argv[0].is<string>())
    throw cedar::make_exception("cedar/symbol requires a :string");
  cedar::runes s = argv[0].to_string(true);
  return new_obj<symbol>(s);
}

cedar_binding(cedar_readstring) {
  ERROR_IF_ARGS_PASSED_IS("read-string", !=, 1);
  if (!argv[0].is<string>()) {
    throw cedar::make_exception("read-string requires a string");
  }
  auto read_results = cedar::reader().run(argv[0].to_string(true));
  if (read_results.size() == 0) return nullptr;
  ref v = new vector();

  for (auto val : read_results) {
    v = self_call(v, "put", val);
  }
  return v;
}

cedar_binding(cedar_read) {
  ERROR_IF_ARGS_PASSED_IS("read", !=, 0);
  std::string line;
  if (std::getline(std::cin, line)) {
    auto read_results = cedar::reader().run(line);
    if (read_results.size() == 0) return nullptr;
    return read_results[0];
  }
  return new_obj<keyword>(":EOF");
}

cedar_binding(cedar_str) {
  cedar::runes s;
  for (int i = 0; i < argc; i++) {
    s += argv[i].to_string(true);
  }
  return new_obj<string>(s);
}


// cedar_throw is a simple wrapper around the c++ exception system
cedar_binding(cedar_throw) {
  ERROR_IF_ARGS_PASSED_IS("throw", !=, 1);
  throw argv[0];
}

cedar_binding(cedar_apply) {
  ERROR_IF_ARGS_PASSED_IS("apply", !=, 2);
  ref f = argv[0];

  if (lambda *fnc = ref_cast<lambda>(f); fnc != nullptr) {
    std::vector<ref> args;
    ref c = argv[1];
    int i = 0;
    for (ref c = argv[1]; !c.is_nil(); c = c.rest()) {
      i++;
      args.push_back(c.first());
    }

    if (fnc->code_type == lambda::function_binding_type) {
      return fnc->function_binding(i, args.data(), ctx);
    }

    fnc = fnc->copy();
    fnc->prime_args(i, args.data());
    return eval_lambda(fnc);
  } else {
    throw cedar::make_exception("(apply ...) to '", f,
                                "' failed because it's not a function");
  }
}

cedar_binding(cedar_rand) {
  double r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
  return r;
}

cedar_binding(cedar_catch) {
  ERROR_IF_ARGS_PASSED_IS("catch", !=, 2);
  ref exceptional = argv[0];
  ref handler = argv[1];

  auto handle = [&](ref e) {
    if (lambda *fn = ref_cast<lambda>(handler); fn != nullptr) {
      fn = fn->copy();
      int ac = 1;
      ref *av = &e;

      fn->prime_args(ac, av);
      return eval_lambda(fn);
    } else {
      throw make_exception("catch requires a lambda as a handler");
    }
  };

  try {
    if (lambda *fn = ref_cast<lambda>(exceptional); fn != nullptr) {
      fn = fn->copy();
      fn->prime_args(0, nullptr);
      return eval_lambda(fn);
    } else {
      throw make_exception("catch requires a lambda as a first argument");
    }

  } catch (ref e) {
    return handle(e);
  } catch (std::exception &c_exception) {
    ref e = new_obj<string>(runes(c_exception.what()));
    return handle(e);
  }

  return nullptr;
}

struct url_data {
  size_t size;
  char *data;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data) {
  size_t index = data->size;
  size_t n = (size * nmemb);
  char *tmp;

  data->size += (size * nmemb);

#ifdef DEBUG
  fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
#endif
  tmp = (char *)realloc(data->data, data->size + 1); /* +1 for '\0' */

  if (tmp) {
    data->data = tmp;
  } else {
    if (data->data) {
      free(data->data);
    }
    fprintf(stderr, "Failed to allocate memory.\n");
    return 0;
  }

  memcpy((data->data + index), ptr, n);
  data->data[data->size] = '\0';

  return size * nmemb;
}



type *reader_type;




std::vector<cedar::runes> regex_matches(
    std::string r, std::string s,
    std::regex_constants::syntax_option_type opts) {
  std::vector<cedar::runes> v;

  std::regex rgx(r, std::regex_constants::ECMAScript | opts);

  std::smatch matches;


  auto search_start(s.cbegin());

  while (std::regex_search(search_start, s.cend(), matches, rgx)) {
    v.push_back(matches[0].str());
    search_start = matches.suffix().first;
  }

  return v;
}




void init_binding(cedar::vm::machine *m) {
  true_value = new symbol("true");

  def_global("list", cedar_newlist);
  def_global("dict", cedar_newdict);
  def_global("vector", cedar_newvector);
  def_global("get", cedar_get);
  def_global("set", cedar_set);
  def_global("keys", cedar_dict_keys);
  def_global("size", cedar_size);
  def_global("+", cedar_add);
  def_global("-", cedar_sub);
  def_global("*", cedar_mul);
  def_global("/", cedar_div);
  def_global("mod", cedar_mod);
  def_global("**", cedar_pow);
  def_global("=", cedar_equal);
  def_global("eq", cedar_equal);
  def_global("is", cedar_is);
  def_global("print", cedar_print);
  def_global("println", cedar_println);
  def_global("cedar/hash", cedar_hash);
  def_global("cons", cedar_cons);
  def_global("<", cedar_lt);
  def_global("<=", cedar_lte);
  def_global(">", cedar_gt);
  def_global(">=", cedar_gte);
  def_global("os-open", cedar_os_open);
  def_global("os-write", cedar_os_write);
  def_global("os-read", cedar_os_read);
  def_global("os-close", cedar_os_close);
  def_global("os-chmod", cedar_os_chmod);
  def_global("cedar/keyword", cedar_keyword);
  def_global("read-string", cedar_readstring);
  def_global("read", cedar_read);
  def_global("cedar/symbol", cedar_symbol);
  def_global("str", cedar_str);
  def_global("throw", cedar_throw);
  def_global("apply", cedar_apply);
  def_global("cedar/rand", cedar_rand);
  def_global("catch*", cedar_catch);
  def_global("dir", obj_dir);




  def_global("repr", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("repr", !=, 1);
    return new string(argv[0].to_string(false));
  });



  def_global("sqrt", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("sqrt", !=, 1);
    return sqrt(argv[0].to_float());
  });


  def_global("ex/type",
             bind_lambda(argc, argv, machine) { return argv[0]->m_type; });


  def_global("getattr*", bind_lambda(argc, argv, machine) {
    return argv[0].getattr(argv[1]);
  });
  def_global("setattr*", bind_lambda(argc, argv, machine) {
    argv[0].setattr(argv[1], argv[2]);
    return argv[2];
  });

  def_global("type", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("type", !=, 1);
    return argv[0].get_type();
  });


  def_global("panic", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("panic", !=, 1);

    std::cerr << "PANIC: " << argv[0].to_string(true) << std::endl;
    exit(-1);
    return nullptr;
  });




  def_global("macroexpand-1", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("macroexpand-1", !=, 1);
    ref obj = argv[0];
    return vm::macroexpand_1(obj);
    ;
  });




  def_global("path-resolve", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("resolve-source-file", !=, 2);
    ref p = argv[0];
    if (p.get_type() != string_type) {
      throw cedar::make_exception("resolve-source-file requires string path");
    }
    ref b = argv[1];
    if (b.get_type() != string_type) {
      throw cedar::make_exception(
          "resolve-source-file requires string base hint");
    }
    cedar::runes path_hint = p.as<string>()->get_content();
    apathy::Path base = b.as<string>()->get_content();
    if (base.is_file()) {
      base = base.up();
    }
    return new string(path_resolve(path_hint, base));
  });


  def_global("read-file", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("read-file", !=, 1);
    ref p = argv[0];
    if (p.get_type() != string_type) {
      throw cedar::make_exception("read-file requires string path");
    }
    std::string path = p.to_string(true);
    apathy::Path pth = path;
    if (!pth.is_file()) {
      throw cedar::make_exception("file not found");
    }
    return new string(util::read_file(path.c_str()));
  });


  def_global("re-match", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("re-match", !=, 2);
    ref re = argv[0];
    ref str = argv[1];
    if (re.get_type() != string_type || str.get_type() != string_type) {
      throw cedar::make_exception("(re-match ...) requires strings");
    }
    std::string reg = re.to_string(true);
    std::string src = str.to_string(true);
    ref v = new vector();
    for (auto &c : regex_matches(reg, src, std::regex_constants::icase)) {
      v = self_call(v, "put", new string(c));
    }
    return v;
  });




  def_global("require", bind_lambda(argc, argv, machine) {
    ERROR_IF_ARGS_PASSED_IS("require", !=, 1);
    std::string path = argv[0].to_string(true);
    return require(path);
  });



  def_global("gc", bind_lambda(argc, argv, machine) {
    GC_gcollect();
    return nullptr;
  });



  def_global("go*", bind_lambda(argc, argv, machine) {
    ref fn = argv[0];
    if (fn.get_type() == lambda_type) {
      fiber *fi = new fiber(fn.as<lambda>());
      add_job(fi);
      return fi;
    }
    throw cedar::make_exception("go* requires a lambda as an argument");
  });



#define BIND_CONSTANT(name, val) def_global(new_obj<symbol>(#name), val)
  BIND_CONSTANT(S_IRWXU, 700);  /* RWX mask for owner */
  BIND_CONSTANT(S_IRUSR, 400);  /* R for owner */
  BIND_CONSTANT(S_IWUSR, 200);  /* W for owner */
  BIND_CONSTANT(S_IXUSR, 100);  /* X for owner */
  BIND_CONSTANT(S_IRWXG, 70);   /* RWX mask for group */
  BIND_CONSTANT(S_IRGRP, 40);   /* R for group */
  BIND_CONSTANT(S_IWGRP, 20);   /* W for group */
  BIND_CONSTANT(S_IXGRP, 10);   /* X for group */
  BIND_CONSTANT(S_IRWXO, 7);    /* RWX mask for other */
  BIND_CONSTANT(S_IROTH, 4);    /* R for other */
  BIND_CONSTANT(S_IWOTH, 2);    /* W for other */
  BIND_CONSTANT(S_IXOTH, 1);    /* X for other */
  BIND_CONSTANT(S_ISUID, 4000); /* set user id on execution */
  BIND_CONSTANT(S_ISGID, 2000); /* set group id on execution */
  BIND_CONSTANT(S_ISVTX, 1000); /* save swapped text even after use */

#define V(opt) def_global(new_obj<symbol>(#opt), opt);
  FOREACH_OPEN_OPTION(V);
#undef V

  reader_type = new type("Reader");

  type_init_default_bindings(reader_type);

  class reader_obj : public object {
   public:
    bool read = false;
    cedar::reader m_reader;
    std::vector<ref> m_parsed;
    unsigned long index = 0;
    inline reader_obj() { m_type = reader_type; }
  };

  reader_type->setattr("__alloc__", bind_lambda(argc, argv, machine) {
    return new reader_obj();
  });


  reader_type->set_field("new", bind_lambda(argc, argv, machine) {
    if (argc != 2 || argv[1].get_type() != string_type)
      throw cedar::make_exception("Reader/new requires a string argument");


    reader_obj *self = argv[0].as<reader_obj>();
    string *s = argv[1].as<string>();

    cedar::runes r = s->get_content();

    self->m_parsed = self->m_reader.run(r);

    return nullptr;
  });


  reader_type->set_field("rest", bind_lambda(argc, argv, machine) {
    reader_obj *self = argv[0].as<reader_obj>();

    if (self->index >= self->m_parsed.size() - 1) {
      return nullptr;
    }

    auto *next = new reader_obj();
    next->m_parsed = self->m_parsed;
    next->index = self->index + 1;
    return next;
  });

  reader_type->set_field("first", bind_lambda(argc, argv, machine) {
    reader_obj *self = argv[0].reinterpret<reader_obj *>();

    auto *next = new reader_obj();
    next->m_reader = self->m_reader;
    if (self->index >= self->m_parsed.size()) {
      return nullptr;
    }
    return self->m_parsed[self->index];
  });

  def_global(new symbol("Reader"), reader_type);
}
