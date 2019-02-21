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
#include <iomanip>  // setprecision
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>

#include <sys/resource.h>

#define GC_THREADS
#include <cedar/thread.h>
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
      return eval_lambda(fn);
    };
  }

  throw cedar::make_exception("lambda_wrap requires a lambda object");
}


#ifndef CORE_DIR
#define CORE_DIR "/usr/local/lib/cedar/core"
#endif



int main(int argc, char **argv) {

  init();

  def_global("*cedar-version*", new cedar::string(CEDAR_VERSION));

  srand((unsigned int)time(nullptr));

  def_global("*cwd*", cedar::new_obj<cedar::string>(apathy::Path::cwd().string()));

  auto core_path = apathy::Path(CORE_DIR);
  core_path.append("core.cdr");

  cedar::runes core_entry = path_resolve(core_path.string(), apathy::Path::cwd());


  cvm.eval_file(core_entry);

  try {
    bool interactive = false;
    bool resources = false;


    char c;
    while ((c = getopt(argc, argv, "ihRe:")) != -1) {
      switch (c) {
        case 'h':
          help();
          exit(0);

        case 'i':
          interactive = true;
          break;
        case 'R':
          resources = true;
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


    if (optind == argc) {
      interactive = true;
    }

    if (optind < argc) {
      std::string path = cedar::path_resolve(argv[optind]);


      def_global("*main*", cedar::new_obj<cedar::string>(path));

      def_global("*file*", cedar::new_obj<cedar::string>(path));

      // there are also args, so make that vector...
      ref args = new cedar::vector();

      optind++;
      for (int i = optind; i < argc; i++) {
        args = cedar::idx_append(args, new cedar::string(argv[i]));
      }
      def_global("*args*", args);
      cvm.eval_file(path);
    } else {
      def_global("*main*", ref{nullptr});
    }


    struct rusage usage;

    if (interactive) {
      def_global("*file*", cedar::new_obj<cedar::string>(apathy::Path::cwd().string()));

      cedar::reader repl_reader;

      ref binding = cedar::new_obj<cedar::symbol>("$$");
      while (interactive) {
        std::string ps1;


        if (resources) {
          getrusage(RUSAGE_SELF, &usage);

          double used_b = usage.ru_maxrss;
          double used_mb = used_b / 1000.0 / 1000.0;

          std::stringstream stream;
          stream << std::fixed << std::setprecision(2) << used_mb;
          ps1 += stream.str();
          ps1 += " MiB used";
        }


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

          def_global(binding, res);
          std::cout << "\x1B[33m" << res << "\x1B[0m" << std::endl;
        } catch (std::exception &e) {
          std::cerr << "Uncaught Exception: " << e.what() << std::endl;
        }
      }
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
static void usage(void) { printf("usage: cedar [-ih] [-e expression] [files] [args...]\n"); }




// print out the help
static void help(void) {
  printf("cedar lisp v%s\n", CEDAR_VERSION);
  printf("  ");
  usage();
  printf("Flags:\n");
  printf("  -i Run in an interactive repl\n");
  printf("  -h Show this help menu\n");
  printf("  -e Evaluate an expression\n");
  printf("  -R Display resource usage at the REPL (debug)\n");
  printf("\n");
}

