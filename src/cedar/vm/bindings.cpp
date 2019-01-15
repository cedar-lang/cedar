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

using namespace cedar;

cedar_binding(cedar_hash) {
  if (argc != 1)
    throw cedar::make_exception("(hash ...) requires one argument, given ",
                                argc);
  return ref((i64)argv[0].hash());
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
  cedar::runes kw_str = ":";
  kw_str += argv[0].get_first().object_type_name();
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

cedar_binding(cedar_pair) {
  if (argc != 2)
    throw cedar::make_exception("(pair ...) requires two arguments, given ",
                                argc);
  ref l = new_obj<cedar::list>();
  l.set_first(argv[0]);
  l.set_rest(argv[1]);
  return l;
}

cedar_binding(cedar_newdict) {
  if ((argc & 1) == 1)
    throw cedar::make_exception(
        "(dict ...) requires an even number of arguments, given ", argc);
  ref d = cedar::new_obj<cedar::dict>();
  for (int i = 0; i < argc; i += 1) {
    dict_set(d, argv[i], argv[i + 1]);
  }
  return d;
}

cedar_binding(cedar_dict_get) {
  if (argc < 2)
    throw cedar::make_exception("(get dict field [default]) requires two or three arguments, given ",
                                argc);
  ref d = argv[0];
  ref k = argv[1];
  try {
    return dict_get(d, k);
  } catch (...) {
    if (argc == 3) return argv[2];
    throw cedar::make_exception("dict has no value at key, '", k, "'");
  }
}

cedar_binding(cedar_dict_set) {
  if (argc != 3)
    throw cedar::make_exception("(set ...) requires two arguments, given ",
                                argc);
  // set the 0th argument's dict entry at the 1st argument to the second
  // argument
  dict_set(argv[0], argv[1], argv[2]);
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

void init_binding(cedar::vm::machine *m) {
  m->bind("dict", cedar_newdict);
  m->bind("get", cedar_dict_get);
  m->bind("set", cedar_dict_set);
  m->bind("keys", cedar_dict_keys);

  m->bind("+", cedar_add);
  m->bind("-", cedar_sub);
  m->bind("*", cedar_mul);
  m->bind("/", cedar_div);

  m->bind("=", cedar_equal);
  m->bind("eq", cedar_equal);

  m->bind("type-of", cedar_typeof);
  m->bind("print", cedar_print);
  m->bind("hash", cedar_hash);

  m->bind("first", cedar_first);
  m->bind("rest", cedar_rest);

  m->bind("pair", cedar_pair);

  m->bind("<", cedar_lt);
  m->bind("<=", cedar_lte);
  m->bind(">", cedar_gt);
  m->bind(">=", cedar_gte);
}
