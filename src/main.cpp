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

/*
 * this file is the entry point for the `cedar` command line utility, as cedar
 * is a library that can be dynamically linked into any c++ program. Because of
 * this, the primary cedar program is dynamically linked to libcedar.so,
 * wherever that might be.
 */

#include <cedar.h>
#include <cedar/lib/linenoise.h>
#include <ctype.h>
#include <dlfcn.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <map>

using ref = cedar::ref;
class dynamic_library : public cedar::object {
  void *m_handle = nullptr;

 public:
  dynamic_library(void *handle) : m_handle(handle){};

  cedar::bound_function find_func(const char *name) {
    void *loc = dlsym(m_handle, name);
    if (loc == nullptr) {
      throw cedar::make_exception("Unable to find function with name ", name);
    }
    return (cedar::bound_function)loc;
  }
  ref to_number() {
    throw cedar::make_exception(
        "Attempt to cast dynamic_library to number failed");
  }

  u64 hash(void) { return reinterpret_cast<u64>(m_handle); }

  inline const char *object_type_name(void) { return "dynamic-library"; };

 protected:
  cedar::runes to_string(bool human = false) {
    char addr_buf[30];
    std::sprintf(addr_buf, "%p", (void *)m_handle);
    cedar::runes str;
    str += "<dynamic-library ";
    str += addr_buf;
    str += ">";
    return str;
  };
};

cedar_binding(cedar_dlopen) {
  if (argc != 1)
    throw cedar::make_exception("(dlopen ...) requires one argument, given ",
                                argc);
  cedar::string *name = cedar::ref_cast<cedar::string>(argv[0]);

  if (name == nullptr) {
    throw cedar::make_exception(
        "dlopen requires a string as the first argument");
  }

  std::string filename = name->get_content();

  void *binding = dlopen(filename.c_str(), RTLD_LAZY);
  if (binding == nullptr)
    throw cedar::make_exception("Dynamic Library not found: ", name);
  return cedar::new_obj<dynamic_library>(binding);
};

cedar_binding(cedar_dlsym) {
  if (argc != 2)
    throw cedar::make_exception("(dlsym ...) requires two arguments, given ",
                                argc);

  dynamic_library *dynlib = cedar::ref_cast<dynamic_library>(argv[0]);
  cedar::string *name = cedar::ref_cast<cedar::string>(argv[1]);
  if (dynlib == nullptr || name == nullptr) {
    throw cedar::make_exception(
        "invalid types passed to dlsym, requires :dynamic_library and :string");
  }
  auto stdname = std::string(name->get_content());
  auto func_binding = dynlib->find_func(stdname.c_str());
  ref func = cedar::new_obj<cedar::lambda>(func_binding);
  return func;
}
static void usage(void);
static void help(void);




using ctx_ptr = std::shared_ptr<cedar::context>;

void daemon_handle_connection(ctx_ptr ctx, int cfd) {


  while (true) {
    char readv[9];
    for (int i = 0; i < 9; i++) readv[i] = 0;
    std::string buf;

    char data;
    ssize_t data_read;

    while ((data_read = recv(cfd, &data, 1, 0)) > 0 && data != '\n')
        buf += data;

    if (data_read == -1) goto discon;



    try {
      cedar::runes expr = buf;
      cedar::ref res = ctx->eval_string(expr);
      std::string resp = res.to_string();
      send(cfd, "200 OK\n", 4, 0);
      send(cfd, resp.c_str(), resp.size(), 0);
    } catch (std::exception & e) {
      send(cfd, "400 ERR\n", 4, 0);
      send(cfd, e.what(), std::strlen(e.what()), 0);
    }
    send(cfd, "\n", 1, 0);
  }

discon:

  close(cfd);
}


void cedar_daemon(ctx_ptr ctx, int port) {
  int m_socket = socket(AF_INET, SOCK_STREAM, 0);
  // define the server address structure
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = INADDR_ANY;
  int s = bind(m_socket, (struct sockaddr *)&server_address,
               sizeof(server_address));
  if (s != 0) {
    std::cerr << std::strerror(errno) << std::endl;
    exit(0);
  }
  listen(m_socket, 20);

  int new_sock;
  while ((new_sock = accept(m_socket, 0, 0)) != -1) {
    std::thread handler(&daemon_handle_connection, ctx, new_sock);
    handler.detach();
  }
  close(m_socket);
}


int main(int argc, char **argv) {

  srand((unsigned int)time(nullptr));
  try {
    auto ctx = std::make_shared<cedar::context>();
    ctx->init();

    ctx->m_evaluator->bind("dlopen", *cedar_dlopen);
    ctx->m_evaluator->bind("dlsym", *cedar_dlsym);

    bool interactive = false;
    bool daemon = false;

    std::thread daemon_thread;
    char c;
    while ((c = getopt(argc, argv, "id:he:")) != -1) {
      switch (c) {
        case 'h':
          help();
          exit(0);

        case 'd': {
          daemon = true;
          int port = atoi(optarg);
          daemon_thread = std::thread(cedar_daemon, ctx, port);
        } break;

        case 'i':
          interactive = true;
          break;

          // TODO: implement evaluate argument
        case 'e': {
          cedar::runes expr = optarg;
          ctx->eval_string(expr);
          break;
        };
        default:
          usage();
          exit(-1);
          break;
      }
    }
    for (int index = optind; index < argc; index++) {
      ctx->eval_file(argv[index]);
    }

    while (interactive) {
      char *buf = linenoise("* ");
      if (buf == nullptr) break;

      cedar::runes b = buf;
			linenoiseHistoryAdd(buf);
      free(buf);
      if (b.size() == 0) continue;
      // b += "\n";
      try {
        ref res = ctx->eval_string(b);
        std::cout << "=> " << res << std::endl;
      } catch (std::exception &e) {
        std::cerr << "err: " << e.what() << std::endl;
      }
    }

    // wait for the daemon thread
    if (daemon) {
      daemon_thread.join();
    }

  } catch (std::exception &e) {
    std::cerr << "Fatal Exception: " << e.what() << std::endl;
    exit(-1);
  } catch (...) {
    printf("unknown\n");
    exit(-1);
  }

  return 0;
}

// print out the usage
static void usage(void) {
  printf("usage: cedar [-ih] [-e expression] [files ...]\n");
}

// print out the help
static void help(void) {
  printf("cedar lisp v%s\n", CEDAR_VERSION);
  printf("  ");
  usage();
  printf("Flags:\n");
  printf("  -i Run in an interactive repl\n");
  printf("  -h Show this help menu\n");
  printf("  -e Evaluate an expression\n");
  printf("\n");
}

