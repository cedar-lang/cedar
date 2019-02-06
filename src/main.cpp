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

#include <apathy.h>
#include <cedar.h>
#include <cedar/lib/linenoise.h>
#include <ctype.h>
#include <dlfcn.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <uv.h>
#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>
#include <uvw.hpp>

#define GC_THREADS
#include <gc/gc.h>

#include <cedar/util.hpp>

cedar::vm::machine cvm;

std::thread start_daemon_thread(short);

using ref = cedar::ref;

static void help(void);
static void usage(void);

using namespace cedar;

cedar::type::method lambda_wrap(ref func) {
  if (lambda *fn = ref_cast<lambda>(func); fn != nullptr) {
    return [func](int argc, ref *argv, vm::machine *m) -> ref {
      lambda *fn = ref_cast<lambda>(func)->copy();
      fn->prime_args(argc, argv);
      return m->eval_lambda(fn);
    };
  }

  throw cedar::make_exception("lambda_wrap requires a lambda object");
}

#ifndef CORE_DIR
#define CORE_DIR "/usr/local/lib/cedar/core"
#endif

int main(int argc, char **argv) {
  cvm.bind(new cedar::symbol("*cedar-version*"),
           new cedar::string(CEDAR_VERSION));

  srand((unsigned int)time(nullptr));

  /*

  cvm.bind(cedar::new_obj<cedar::symbol>("*cwd*"),
           cedar::new_obj<cedar::string>(apathy::Path::cwd().string()));
           */

  auto core_path = apathy::Path(CORE_DIR);
  core_path.append("core.cdr");

  cedar::runes core_entry =
      path_resolve(core_path.string(), apathy::Path::cwd());

    cvm.eval_file(core_entry);



  try {
    bool interactive = false;
    bool daemon = false;

    std::thread daemon_thread;

    char c;
    while ((c = getopt(argc, argv, "iD:he:")) != -1) {
      switch (c) {
        case 'h':
          help();
          exit(0);

        case 'D': {
          // daemon = true;
          // daemon_thread = start_daemon_thread(atoi(optarg));
        } break;

        case 'i':
          interactive = true;
          break;

          // TODO: implement evaluate argument
        case 'e': {
          cedar::runes expr = optarg;
          cvm.eval_string(expr);
          return 0;
          break;
        };
        default:
          usage();
          exit(-1);
          break;
      }
    }


    if (optind == argc && !daemon) {
      interactive = true;
    }

    if (optind < argc) {
      std::string path = cedar::path_resolve(argv[optind]);

      cvm.bind(cedar::new_obj<cedar::symbol>("*main*"),
               cedar::new_obj<cedar::string>(path));

      cvm.bind(cedar::new_obj<cedar::symbol>("*file*"),
               cedar::new_obj<cedar::string>(path));

      // there are also args, so make that vector...
      ref args = new cedar::vector();

      optind++;
      for (int i = optind; i < argc; i++) {
        args = cedar::idx_append(args, new cedar::string(argv[i]));
      }
      cvm.bind(cedar::new_obj<cedar::symbol>("*args*"), args);
      cvm.eval_file(path);
    }


    // the repl
    int repl_ind = 0;
    if (interactive) {
      /*
      cvm.bind(cedar::new_obj<cedar::symbol>("*file*"),
               cedar::new_obj<cedar::string>(apathy::Path::cwd().string()));

      */
      printf("\n");
      printf("cedar lisp v%s\n", CEDAR_VERSION);
      cedar::reader repl_reader;

      while (interactive) {
        std::string ps1;
        ps1 += "> ";
        char *buf = linenoise(ps1.data());
        if (buf == nullptr) {
          printf("\x1b[1A");
          break;
        }
        cedar::runes b = buf;

        if (b.size() == 0) {
          free(buf);
          continue;
        }

        linenoiseHistoryAdd(buf);
        free(buf);
        try {
          printf("\x1B[32m");
          ref res = cvm.eval_string(b);
          printf("\x1B[0m");

          cedar::runes name = "$";
          name += std::to_string(repl_ind++);
          ref binding = cedar::new_obj<cedar::symbol>(name);
          cvm.bind(binding, res);
          std::cout << name << ": "
                    << "\x1B[33m" << res << "\x1B[0m" << std::endl;
        } catch (std::exception &e) {
          std::cerr << "Uncaught Exception: " << e.what() << std::endl;
        }
      }
    }

    // wait for the daemon thread
    if (daemon) {
      daemon_thread.join();
    }

  } catch (std::exception &e) {
    std::cerr << "Uncaught exception: " << e.what() << std::endl;
    exit(-1);
  } catch (cedar::ref r) {
    std::cerr << "Uncaught exception: " << r << std::endl;
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

void daemon_thread(short port) {
  auto loop = uvw::Loop::getDefault();
  auto tcp = loop->resource<uvw::TCPHandle>();

  tcp->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &e, uvw::TCPHandle &h) {
    //
    std::cerr << e.what() << std::endl;
  });

  tcp->on<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &srv) {
    std::shared_ptr<uvw::TCPHandle> client =
        srv.loop().resource<uvw::TCPHandle>();

    std::string welcome = "cedar lisp v" CEDAR_VERSION "\n";

    client->on<uvw::ConnectEvent>(
        [&](const uvw::ConnectEvent &e, uvw::TCPHandle &h) {});

    client->on<uvw::DataEvent>([&](const uvw::DataEvent &e, uvw::TCPHandle &h) {
      /* data received */
      cedar::runes str = e.data.get();

      std::string headers;
      std::string body;

      try {
        ref res = cvm.eval_string(str);
        headers += "200 OKAY\n";
        body += res.to_string();

      } catch (std::exception &e) {
        headers += "500 EXCEPTION\n";

        body += e.what();
      } catch (cedar::ref r) {
        headers += "501 UNCAUGHT\n";

        body += "Uncaught exception: ";
        body += r.to_string();
      }

      std::string response;
      response += headers;
      response += "\n";
      response += body;
      response += "\n";

      h.write(response.data(), response.size());
    });

    client->on<uvw::EndEvent>(
        [](const uvw::EndEvent &e, uvw::TCPHandle &client) {
          //
          client.close();
        });

    srv.accept(*client);
    client->write(welcome.data(), welcome.size());
    client->read();
  });

  tcp->init();
  tcp->bind("127.0.0.1", port);
  printf("Daemon Listening on 127.0.0.1:%d\n", port);
  tcp->listen();
  loop->run();
}

std::thread start_daemon_thread(short port) {
  auto th = std::thread(daemon_thread, port);
  return th;
}
