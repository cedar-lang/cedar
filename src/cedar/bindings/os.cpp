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

#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/dict.h>
#include <cedar/object/fiber.h>
#include <cedar/object/keyword.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/vector.h>
#include <cedar/objtype.h>
#include <cedar/vm/binding.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mutex>


using namespace cedar;



static cedar_binding(os_which) {
#ifdef __linux__
  return new keyword(":linux");
#elif __APPLE__
  return new keyword(":macos");
#elif _WIN32
  return new keyword(":windows");
#else
  return new keyword(":posix");
#endif
}

static cedar_binding(os_fork) { return fork(); }

static cedar_binding(os_shell) {
  if (argc != 1) throw cedar::make_exception("os.shell requires 1 argument");

  if (argv[0].get_type() != string_type) {
    throw cedar::make_exception("os.shell requires a string as an argument");
  }

  std::string cmd = argv[0].to_string(true);

  int stat = std::system(cmd.c_str());
  return stat;
}


static cedar_binding(os_stat) {
  if (argc != 1) throw cedar::make_exception("os.stat requires 1 argument");
  if (argv[0].get_type() != string_type) {
    throw cedar::make_exception(
        "os.stat requires a string path as an argument");
  }
  std::string path = argv[0].to_string(true);
  struct stat s;
  int stat_res = stat(path.c_str(), &s);

#define STAT_ITER(V) \
  V(mode)            \
  V(ino)             \
  V(dev)             \
  V(nlink)           \
  V(uid)             \
  V(gid)             \
  V(size)            \
  V(atime)           \
  V(mtime)           \
  V(ctime)

#define V(name) static ref name = new keyword(":" #name);
  STAT_ITER(V)
#undef V

  ref d = new dict();
#define V(name) d = self_call(d, "set", name, (i64)s.st_##name);
  STAT_ITER(V)
#undef V

  d = self_call(d, "set", new keyword(":res"), stat_res);

#undef STAT_ITER

  return d;
}



static cedar_binding(os_listdir) {
  std::string path = ".";

  if (argc == 1) {
    if (argv[0].get_type() != string_type) {
      throw cedar::make_exception(
          "os.listdir requires a string path as an argument");
    }
    path = argv[0].to_string(true);
  }
  ref files = new vector();
  DIR *d;
  struct dirent *dir;
  d = opendir(path.c_str());
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      files = self_call(files, "put", new string(dir->d_name));
    }
    closedir(d);
  }
  return files;
}

static cedar_binding(os_rm) {
  static ref _true = new symbol("true");
  static ref _false = new symbol("false");
  if (argc != 1) throw cedar::make_exception("os.rm requires 1 argument");
  if (argv[0].get_type() != string_type) {
    throw cedar::make_exception("os.rm requires a string path as an argument");
  }
  std::string path = argv[0].to_string(true);
  return remove(path.c_str()) == 0 ? _true : _false;
}



typedef void(init_func)(void);
static cedar_binding(os_import_so) {
  if (argc != 1)
    throw cedar::make_exception("os.import-so requires 1 argument");

  if (argv[0].get_type() != string_type) {
    throw cedar::make_exception(
        "os.import-so requires a string as an argument");
  }

  std::string path = argv[0].to_string(true);


  void *handle = dlopen(path.c_str(), RTLD_NOW);
  if (handle == nullptr) {
    throw cedar::make_exception("unable to find shared object file");
  }

  init_func *init = (init_func *)dlsym(handle, "$CDR-INIT$");

  if (init == nullptr) {
    throw cedar::make_exception(
        "unable to find init function in shared object file");
  }

  init();
  return nullptr;
}

static cedar_binding(os_getpid) { return (i64)getpid(); }

static cedar_binding(os_getppid) { return (i64)getppid(); }

static cedar_binding(os_getenv) {
  if (argc != 1) throw cedar::make_exception("os.getenv requires 1 argument");
  if (argv[0].get_type() != string_type) {
    throw cedar::make_exception("os.getenv requires a string as an argument");
  }
  std::string path = argv[0].to_string(true);
  const char *env = getenv(path.c_str());
  if (env) {
    return new string(env);
  }
  return nullptr;
}


static ref os_panic(int argc, ref *argv, call_context *ctx) {
  if (argc != 1) throw cedar::make_exception("os.panic requires 1 argument");

  auto *f = ctx->coro;


  std::cout << "panic: " << argv[0].to_string(true) << std::endl;
  std::cout << "\n";
  f->print_callstack();
  std::cout << "\n";
  exit(-1);

  return nullptr;
}


void bind_os(void) {
  module *mod = new module("os");

  mod->def("shell", os_shell);
  mod->def("fork", os_fork);
  mod->def("shell", os_shell);
  mod->def("fork", os_fork);
  mod->def("import-so", os_import_so);
  mod->def("which", os_which);
  mod->def("listdir", os_listdir);
  mod->def("stat", os_stat);
  mod->def("rm", os_rm);
  mod->def("getpid", os_getpid);
  mod->def("getppid", os_getppid);
  mod->def("getenv", os_getenv);
  mod->def("panic", os_panic);

  define_builtin_module("os-internal", mod);
}
