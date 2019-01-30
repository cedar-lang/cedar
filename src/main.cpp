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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>








using ref = cedar::ref;






static void help(void);
static void usage(void);

int main(int argc, char **argv) {

  cedar::vm::machine vm;

  vm.bind(new cedar::symbol("*cedar-version*"), new cedar::string(CEDAR_VERSION));

  srand((unsigned int)time(nullptr));
  try {

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
          // int port = atoi(optarg);
          // daemon_thread = std::thread(cedar_daemon, ctx, port);
        } break;

        case 'i':
          interactive = true;
          break;

          // TODO: implement evaluate argument
        case 'e': {
          cedar::runes expr = optarg;
          vm.eval_string(expr);
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
    for (int index = optind; index < argc; index++) {
      vm.eval_file(argv[index]);
    }



    // the repl
    int repl_ind = 0;
    if (interactive) {
      printf("cedar lisp v%s\n", CEDAR_VERSION);
      printf("\n");
      cedar::reader repl_reader;

      while (interactive) {
        char *buf = linenoise("> ");
        if (buf == nullptr) break;

        cedar::runes b = buf;

        if (b.size() == 0) {
          free(buf);
          continue;
        }

        linenoiseHistoryAdd(buf);
        free(buf);
        try {
          ref res = vm.eval_string(b);
          cedar::runes name = "$";
          name += std::to_string(repl_ind++);
          ref binding = cedar::new_obj<cedar::symbol>(name);
          vm.bind(binding, res);
          std::cout << name << ": " << "\x1B[33m" << res << "\x1B[0m" << std::endl;
        } catch (std::exception &e) {
          std::cerr << "err: " << e.what() << std::endl;
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

