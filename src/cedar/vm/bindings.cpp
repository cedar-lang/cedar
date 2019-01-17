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
#include <cedar/vm/binding.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace cedar;

#define ERROR_IF_ARGS_PASSED_IS(name, op, c)                    \
  if (argc op c)                                                \
    throw cedar::make_exception("(" #name " ...) requires ", c, \
                                " arg", (c == 1 ? "" : "s"), "given ", argc);

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
    return argv[0].reinterpret<u64>() == argv[0].reinterpret<u64>() ? machine->true_value : nullptr;
  return argv[0] == argv[0] ? machine->true_value : nullptr;
}

cedar_binding(cedar_add) {
  ref accumulator = 0;
  for (int i = 0; i < argc; i++) {
    accumulator = accumulator + argv[i];
  }
  return accumulator;
}

cedar_binding(cedar_sub) {
  if (argc == 1) {
    return ref{-1} * argv[0];
  }
  ref acc = argv[0];
  for (int i = 1; i < argc; i++) {
    acc = acc - argv[i];
  }
  return acc;
}

cedar_binding(cedar_mul) {
  ref acc = 1;
  for (int i = 0; i < argc; i++) {
    acc = acc * argv[i];
  }
  return acc;
}

cedar_binding(cedar_div) {
  if (argc == 1) {
    return ref{1} / argv[0];
  }
  ref acc = argv[0];
  for (int i = 1; i < argc; i++) {
    acc = acc / argv[i];
  }
  return acc;
}

cedar_binding(cedar_equal) {
  if (argc < 2)
    throw cedar::make_exception(
        "(= ...) requires at least two arguments, given ", argc);
  ref first = argv[0];
  for (int i = 1; i < argc; i++) {
    if (argv[i] != first) return nullptr;
  }
  return machine->true_value;
}

cedar_binding(cedar_lt) {
  return argv[0] < argv[1] ? machine->true_value : nullptr;
}
cedar_binding(cedar_lte) {
  return argv[0] <= argv[1] ? machine->true_value : nullptr;
}
cedar_binding(cedar_gt) {
  return argv[0] > argv[1] ? machine->true_value : nullptr;
}
cedar_binding(cedar_gte) {
  return argv[0] >= argv[1] ? machine->true_value : nullptr;
}

cedar_binding(cedar_print) {
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i].to_string(true) << " ";
  }
  std::cout << std::endl;
  return 0;
}

cedar_binding(cedar_typeof) {
  if (argc != 1)
    throw cedar::make_exception("(type-of ...) requires one argument, given ",
                                argc);

  if (argv[0].is<cedar::dict>()) {
    try {
      return idx_get(argv[0], new_obj<keyword>(":type-of"));
    } catch (...) {
      return new_obj<keyword>(":dict");
    }
  }
  cedar::runes kw_str = ":";
  kw_str += argv[0].object_type_name();
  return new_obj<cedar::keyword>(kw_str);
}

cedar_binding(cedar_first) {
  if (argc != 1)
    throw cedar::make_exception("(first ...) requires one argument, given ",
                                argc);
  return argv[0].get_first();
}

cedar_binding(cedar_rest) {
  if (argc != 1)
    throw cedar::make_exception("(rest ...) requires one argument, given ",
                                argc);
  return argv[0].get_rest();
}

cedar_binding(cedar_cons) {
  if (argc != 2)
    throw cedar::make_exception("(cons ...) requires two argument, given ",
                                argc);
  return new_obj<list>(argv[0], argv[1]);
}


cedar_binding(cedar_newlist) {
  ref l = new_obj<list>();
  ref base = l;
  for (int i = 0; i < argc; i++) {
    l.set_first(argv[i]);
    if (i < argc-1) {
      l.set_rest(new_obj<list>());
      l = l.get_rest();
    }
  }

  return base;
}

cedar_binding(cedar_newdict) {
  if ((argc & 1) == 1)
    throw cedar::make_exception(
        "(dict ...) requires an even number of arguments, given ", argc);

  ref d = cedar::new_obj<cedar::dict>();
  for (int i = 0; i < argc; i += 1) {
    idx_set(d, argv[i], argv[i + 1]);
  }
  return d;
}

cedar_binding(cedar_get) {
  if (argc < 2)
    throw cedar::make_exception(
        "(get dict field [default]) requires two or three arguments, given ",
        argc);
  ref d = argv[0];
  ref k = argv[1];
  try {
    return idx_get(d, k);
  } catch (...) {
    if (argc == 3) return argv[2];
    throw cedar::make_exception("dict has no value at key, '", k, "'");
  }
}

cedar_binding(cedar_set) {
  if (argc != 3)
    throw cedar::make_exception("(set ...) requires two arguments, given ",
                                argc);
  // set the 0th argument's dict entry at the 1st argument to the second
  // argument
  idx_set(argv[0], argv[1], argv[2]);
  return argv[2];
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


cedar_binding(cedar_refcount) {
  if (argc != 1)
    throw cedar::make_exception("(refcount ...) requires one arguments, given ",
                                argc);

  if (argv[0].is_obj()) {
    if (object *o = ref_cast<object>(argv[0]); o != nullptr) {
      return (i64)o->refcount;
    }
  }
  return 1;
}

// give a macro expansion for each open option argument
#define FOREACH_OPEN_OPTION(V) \
  V(O_RDONLY)                  \
  V(O_WRONLY)                  \
  V(O_RDWR)                    \
  V(O_NONBLOCK)                \
  V(O_APPEND)                  \
  V(O_CREAT)                   \
  V(O_TRUNC)                   \
  V(O_EXCL)                    \
  V(O_SHLOCK)                  \
  V(O_EXLOCK)                  \
  V(O_NOFOLLOW)                \
  V(O_SYMLINK)                 \
  V(O_EVTONLY)                 \
  V(O_CLOEXEC)

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
  return (i64)open(path_s.c_str(), flags);
}

cedar_binding(cedar_os_write) {
  ERROR_IF_ARGS_PASSED_IS("os-write", !=, 2);
  std::string buf = argv[1].to_string(true);
  return (i64)write(argv[0].to_int(), buf.c_str(), buf.size());
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

cedar_binding(cedar_binary_or) {
  ERROR_IF_ARGS_PASSED_IS("bit-or", <, 2);
  i64 res = argv[0].to_int();
  for (int i = 1; i < argc; i++) res |= argv[i].to_int();
  return res;
}

cedar_binding(cedar_binary_and) {
  ERROR_IF_ARGS_PASSED_IS("bit-and", <, 2);
  i64 res = argv[0].to_int();
  for (int i = 1; i < argc; i++) res &= argv[i].to_int();
  return res;
}

cedar_binding(cedar_binary_xor) {
  ERROR_IF_ARGS_PASSED_IS("bit-xor", !=, 2);
  i64 res = argv[0].to_int() & argv[1].to_int();
  return res;
}

cedar_binding(cedar_binary_shift_left) {
  ERROR_IF_ARGS_PASSED_IS("bit-shift-left", !=, 2);
  i64 res = argv[0].to_int() << argv[1].to_int();
  return res;
}

cedar_binding(cedar_binary_shift_right) {
  ERROR_IF_ARGS_PASSED_IS("bit-shift-right", !=, 2);
  i64 res = argv[0].to_int() << argv[1].to_int();
  return res;
}

cedar_binding(cedar_newvector) {
  ref v = new_obj<vector>();
  for (int i = 0; i < argc; i++) idx_append(v, argv[i]);
  return v;
}


cedar_binding(cedar_keyword) {
  ERROR_IF_ARGS_PASSED_IS("cedar/symbol", !=, 1);

  cedar::runes s = ":";
  s += argv[0].to_string(true);
  return new_obj<keyword>(s);
}

void init_binding(cedar::vm::machine *m) {
  m->bind("list", cedar_newlist);
  m->bind("dict", cedar_newdict);
  m->bind("vector", cedar_newvector);
  m->bind("get", cedar_get);
  m->bind("set", cedar_set);
  m->bind("keys", cedar_dict_keys);
  m->bind("size", cedar_size);

  m->bind("+", cedar_add);
  m->bind("-", cedar_sub);
  m->bind("*", cedar_mul);
  m->bind("/", cedar_div);

  m->bind("=", cedar_equal);
  m->bind("eq", cedar_equal);

  m->bind("is", cedar_is);
  m->bind("type-of", cedar_typeof);
  m->bind("print", cedar_print);
  m->bind("cedar/hash", cedar_hash);
  m->bind("cedar/id", cedar_id);

  m->bind("first", cedar_first);
  m->bind("rest", cedar_rest);
  m->bind("cons", cedar_cons);

  m->bind("<", cedar_lt);
  m->bind("<=", cedar_lte);
  m->bind(">", cedar_gt);
  m->bind(">=", cedar_gte);

  m->bind("bit-or", cedar_binary_or);
  m->bind("bit-and", cedar_binary_and);
  m->bind("bit-xor", cedar_binary_xor);
  m->bind("bit-shift-left", cedar_binary_shift_left);
  m->bind("bit-shift-right", cedar_binary_shift_right);

  m->bind("os-open", cedar_os_open);
  m->bind("os-write", cedar_os_write);
  m->bind("os-close", cedar_os_close);

  m->bind("os-chmod", cedar_os_chmod);

  m->bind("cedar/keyword", cedar_keyword);
  m->bind("cedar/refcount", cedar_refcount);
#define BIND_CONSTANT(name, val) m->bind(new_obj<symbol>(#name), val)

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

#define V(opt) m->bind(new_obj<symbol>(#opt), opt);
  FOREACH_OPEN_OPTION(V);
#undef V
}
