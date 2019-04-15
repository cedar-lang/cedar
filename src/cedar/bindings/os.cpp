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

#include <cedar/event_loop.h>
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
#include <uv.h>
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


static void os_panic(const function_callback &args) {
  if (args.len() != 1)
    throw cedar::make_exception("os.panic requires 1 argument");

  auto *f = args.get_fiber();


  std::cout << "panic: " << args[0].to_string(true) << std::endl;
  std::cout << "\n";
  f->print_callstack();
  std::cout << "\n";
  exit(-1);
}

static void os_exit(const function_callback &args) {
  if (args.len() >= 1)
    throw cedar::make_exception("os.exit requires 0 or 1 argument");
  if (args.len() == 1) {
    exit(args[0].to_int());
  }
  exit(0);
}




static void os_hardware_concurrency(const function_callback &args) {
  args.get_return() = (int)std::thread::hardware_concurrency();
}



static void on_fs_open(uv_fs_t *req) {
  ref fd;
  ref err;
  if (req->result >= 0) {
    fd = req->result;
    err = nullptr;
  } else {
    fd = -1;
    err = new string(uv_strerror(req->result));
  }

  ref args[2];
  args[0] = fd;
  args[1] = err;

  auto *fn = (lambda *)req->data;

  fiber *f = new fiber(fn->prime(2, args));
  add_job(f);
}

static void os_open_raw(const function_callback &args) {
  if (args.len() != 3)
    throw cedar::make_exception(
        "os.open requires 2 arguments: (os.open-raw path mode-bits callback)");

  std::string s = args[0].to_string(true);
  int mode = args[1].to_int();
  uv_fs_t *open_req = new uv_fs_t();
  open_req->data = (void *)ref_cast<lambda>(args[2]);


  in_ev([=](uv_loop_t *loop) {
    char *p = new char[s.size() + 1];
    memcpy(p, s.c_str(), s.size());
    uv_fs_open(loop, open_req, p, mode, 0, on_fs_open);
  });
}




void os_write_raw(const function_callback &args) {
  // TODO:
}


struct read_req_data {
  lambda *cb;
  uv_buf_t buffer;
};


static void on_read_raw(uv_fs_t *req) {
  auto *data = (read_req_data *)req->data;

  ref content = nullptr;
  ref err = nullptr;

  if (req->result < 0) {
    err = new string(uv_strerror(req->result));
  } else {
    if (req->result != 0) {
      content = new string(data->buffer.base);
    }
  }


  ref args[2];
  args[0] = content;
  args[1] = err;

  auto *fn = data->cb;
  fiber *f = new fiber(fn->prime(2, args));
  add_job(f);
}



void os_read_raw(const function_callback &args) {
  // arg order: (os.read-raw fd count callback)
  if (args.len() != 3)
    return args.throw_obj(new string(
        "os.read-raw requires 3 arguments: (os.read-raw fd count callback)"));

  // there is no argument type checking here.
  // TODO: add 'quick' arg checking
  int fd = args[0].to_int();
  int count = args[1].to_int();
  lambda *cb = ref_cast<lambda>(args[2]);

  uv_fs_t *req = new uv_fs_t();
  auto *data = new read_req_data();
  data->cb = cb;
  data->buffer = uv_buf_init(new char[count + 1], count);
  data->buffer.base[count] = 0;
  req->data = (void *)data;

  if (cb == nullptr) {
    return args.throw_obj(new string("os.read-raw takes a callback function"));
  }

  in_ev([=](uv_loop_t *loop) {
    uv_fs_read(loop, req, fd, &data->buffer, 1, -1, on_read_raw);
  });
}


static void os_timeout_callback(uv_timer_t *handle) {
  lambda *cb = static_cast<lambda *>(handle->data);
  add_job(new fiber(cb->prime()));
}


static void os_timeout(const function_callback &args) {
  // there is no argument type checking here.
  // TODO: add 'quick' arg checking
  int time = args[0].to_int();
  lambda *cb = ref_cast<lambda>(args[1]);

  uv_timer_t *timer = new uv_timer_t();

  timer->data = (void *)cb;

  in_ev([=](uv_loop_t *loop) {
    uv_timer_init(loop, timer);
    uv_timer_start(timer, os_timeout_callback, time, 0);
    uv_update_time(loop);
  });
}


static void os_close(const function_callback &args) {
  if (args.len() != 1) {
    return args.throw_obj(new string("os.close requires 1 argument"));
  }

  close(args[0].to_int());
}
void bind_os(void) {
  module *mod = new module("os-internal");

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
  mod->def("exit", os_exit);
  mod->def("get-ncpus", os_hardware_concurrency);


  // some filesystem related activities
  mod->def("open-raw", os_open_raw);
  mod->def("write-raw", os_write_raw);
  mod->def("read-raw", os_read_raw);
  mod->def("close", os_close);



  mod->def("timeout", os_timeout);


  mod->def("RDONLY", ref((int)O_RDONLY));
  mod->def("WRONLY", O_WRONLY);
  mod->def("RDWR", O_RDWR);
  mod->def("NONBLOCK", O_NONBLOCK);
  mod->def("APPEND", O_APPEND);
  mod->def("CREAT", O_CREAT);
  mod->def("TRUNC", O_TRUNC);
  mod->def("EXCL", O_EXCL);
  mod->def("NOFOLLOW", O_NOFOLLOW);
  mod->def("CLOEXEC", O_CLOEXEC);

  define_builtin_module("os-internal", mod);
}

