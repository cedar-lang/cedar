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

#include <cedar/modules.h>
#include <cedar/parser.h>
#include <cedar/vm/compiler.h>
#include <cedar/object/lambda.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <unordered_map>
#include <mutex>
#include <stdio.h>
#include <apathy.h>
#include <cedar/util.hpp>

using namespace cedar;

static std::vector<std::string> get_path(void) {
  const char *path = getenv("CEDARPATH");
  if (path == nullptr) {
    // return the default path
    std::vector<std::string> path;
    path.push_back(".");
    path.push_back("/usr/local/lib/cedar");
    path.push_back(apathy::Path("~/.local/lib/cedar").absolute().string());
    return path;
  }

  // TODO: handle CEDARPATH
  return {};
}


static std::mutex mod_mutex;
static std::unordered_map<std::string, module*> modules;



static module *require_file(apathy::Path p) {
  std::unique_lock<std::mutex> lock(mod_mutex);
  std::string path = p.string();

  if (modules.count(path) != 0) {
    return modules.at(path);
  }

  cedar::runes src = util::read_file(path.c_str());


  module *mod = new module(path);


  static int file_id = get_symbol_intern_id("*file*");
  mod->setattr_fast(file_id, new string(path));

  eval_string_in_module(src, mod);

  modules[path] = mod;

  return mod;
}



module *cedar::require(std::string name) {
  if (modules.count(name) != 0) {
    return modules.at(name);
  }
  auto path = get_path();

  for (std::string p : path) {
    apathy::Path f = p;
    f.append(name);
    if (f.is_directory()) {
      return require_file(f.append("main.cdr"));
    }
    if (f.is_file()) {
      return require_file(f);
    }
    // well the above stuff didn't work...
    // so lets try adding .cdr to the end :)
    f = p;
    f.append(name + ".cdr");
    if (f.is_file()) {
      return require_file(f);
    }
  }
  return nullptr;
}

// simple function to allow modules to be added externally
void cedar::define_builtin_module(std::string name, module *mod) {
  modules[name] = mod;
}



ref cedar::eval_string_in_module(cedar::runes src, module *mod) {
  reader reader;
  reader.lex_source(src);

  vm::compiler c;
  bool valid = true;

  ref val;
  while (true) {
    ref obj = reader.read_one(&valid);
    if (!valid) break;
    ref compiled_lambda = c.compile(obj, nullptr);
    lambda *raw_program = ref_cast<cedar::lambda>(compiled_lambda);
    raw_program->mod = mod;
    val = eval_lambda(raw_program);
  }

  return val;
}
