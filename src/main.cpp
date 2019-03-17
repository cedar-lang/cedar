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
#include <ctype.h>
#include <dlfcn.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>
#include <uv.h>
#include <chrono>
#include <exception>
#include <iomanip>  // setprecision
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>
#define GC_THREADS
#include <cedar/thread.h>
#include <ffi/ffi.h>
#include <gc/gc.h>
#include <cedar/util.hpp>

#include <replxx/replxx.hxx>

using Replxx = replxx::Replxx;


static void help(void);
static void usage(void);

using namespace cedar;


void hook_color(std::string const &str, Replxx::colors_t &colors, module *mod);



int main(int argc, char **argv) {

  srand((unsigned int)time(nullptr));

  init();

  def_global("*cedar-version*", new cedar::string(CEDAR_VERSION));

  module *repl_mod = new module("user");


  try {
    bool interactive = false;

    char c;

    while ((c = getopt(argc, argv, "ihe:")) != -1) {
      switch (c) {
        case 'h':
          help();
          exit(0);

        case 'i':
          interactive = true;
          break;

          // TODO: implement evaluate argument
        case 'e': {
          cedar::runes expr = optarg;
          eval_string_in_module(expr, repl_mod);
          return 0;
          break;
        };
        default:
          usage();
          exit(-1);
          break;
      }
    }



    // there are also args, so make that vector...
    ref args = new cedar::vector();
    for (int i = optind; i < argc; i++) {
      args = cedar::idx_append(args, new cedar::string(argv[i]));
    }
    require("os")->def("args", args);

    if (optind == argc) {
      interactive = true;
    }

    if (optind < argc) {
      std::string path = argv[optind];

      // there are also args, so make that vector...
      ref args = new cedar::vector();
      optind++;
      for (int i = optind; i < argc; i++) {
        args = self_call(args, "put", new string(argv[i]));
      }
      require("os")->def("args", args);
      repl_mod = require(path);
    }


    if (interactive) {
      Replxx rx;
      printf("cedar lisp v" CEDAR_VERSION "\n");
      rx.history_load("~/.cedar_history");
      using namespace std::placeholders;
      rx.set_highlighter_callback(std::bind(&hook_color, _1, _2, repl_mod));
      repl_mod->def("*file*", cedar::new_obj<cedar::string>(
                                  apathy::Path::cwd().string()));
      cedar::reader repl_reader;
      while (interactive) {
        std::string ps1;
        ps1 += "> ";
        char const *buf = nullptr;
        do {
          buf = rx.input(ps1);
        } while ((buf == nullptr) && (errno == EAGAIN));

        if (buf == nullptr) {
          break;
        }
        std::string input = buf;

        if (input.empty()) {
          continue;
        }
        rx.history_add(input);

        try {
          cedar::runes r = input;
          ref res = eval_string_in_module(r, repl_mod);
          repl_mod->def("$$", res);
          std::cout << "\x1B[33m" << res << "\x1B[0m" << std::endl;

        } catch (std::exception &e) {
          std::cerr << "Uncaught Exception: " << e.what() << std::endl;
        }
      }
    }


    while (!all_work_done()) {
      usleep(200);
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

////////
//
//
//
//
//
//
//
//
//
////////



int utf8_strlen(char const *s, int utf8len) {
  int codepointLen = 0;
  unsigned char m4 = 128 + 64 + 32 + 16;
  unsigned char m3 = 128 + 64 + 32;
  unsigned char m2 = 128 + 64;
  for (int i = 0; i < utf8len; ++i, ++codepointLen) {
    char c = s[i];
    if ((c & m4) == m4) {
      i += 3;
    } else if ((c & m3) == m3) {
      i += 2;
    } else if ((c & m2) == m2) {
      i += 1;
    }
  }
  return (codepointLen);
}


void hook_color(std::string const &context, Replxx::colors_t &colors,
                module *mod) {
  using cl = Replxx::Color;

  static auto cols = std::vector<cl>{cl::BLUE, cl::RED, cl::GREEN};
  /*
  cedar::runes c = context;
  int pi = 0;
  for (u32 i = 0; i < c.size(); i++) {

    bool is_paren = false;
    if (c[i] == '(') {
      pi++;
      is_paren = true;
    } else if (c[i] == ')') {
      pi--;
      is_paren = true;
    }

    if (is_paren) colors.at(i) = cl::BLUE;
  }
  */

  // return;

  std::vector<std::pair<std::string, cl>> regex_color{
      // single chars
      {"\\`", cl::BRIGHTCYAN},
      {"\\'", cl::BRIGHTBLUE},
      {"\\\"", cl::BRIGHTBLUE},
      {"\\-", cl::BRIGHTBLUE},
      {"\\+", cl::BRIGHTBLUE},
      {"\\=", cl::BRIGHTBLUE},
      {"\\/", cl::BRIGHTBLUE},
      {"\\*", cl::BRIGHTBLUE},
      {"\\^", cl::BRIGHTBLUE},
      {"\\.", cl::BRIGHTMAGENTA},
      /*
      {"\\(", cl::NORMAL},
      {"\\)", cl::NORMAL},
      {"\\[", cl::BRIGHTMAGENTA},
      {"\\]", cl::BRIGHTMAGENTA},
      {"\\{", cl::BRIGHTMAGENTA},
      {"\\}", cl::BRIGHTMAGENTA},
      */
      // commands
      {"nil", cl::MAGENTA},
      {"true", cl::BLUE},
      {"false", cl::BLUE},
      {"fn", cl::BLUE},
      // numbers
      {"[\\-|+]{0,1}[0-9]+", cl::YELLOW},           // integers
      {"[\\-|+]{0,1}[0-9]*\\.[0-9]+", cl::YELLOW},  // decimals
      // strings
      {"\"(?:[^\"\\\\]|\\\\.)*\"", cl::BRIGHTGREEN},  // double quotes
      {"\\w+", cl::ERROR},
      {":\\w+", cl::RED},
  };

  // highlight matching regex sequences
  for (auto const &e : regex_color) {
    size_t pos{0};
    std::string str = context;
    std::smatch match;

    while (std::regex_search(str, match, std::regex(e.first))) {
      std::string c{match[0]};
      std::string prefix(match.prefix().str());
      pos += utf8_strlen(prefix.c_str(), static_cast<int>(prefix.length()));
      int len(utf8_strlen(c.c_str(), static_cast<int>(c.length())));


      cl color = e.second;
      // having a color of "error" signals that it's a word
      if (color == cl::ERROR) {
        color = cl::NORMAL;

        bool found = false;
        auto i = symbol::intern(c);
        mod->find(i, &found);

        if (found || is_global(i)) {
          color = cl::BLUE;
        } else if (vm::is_macro(i)) {
          color = cl::BRIGHTMAGENTA;
        }
      }


      for (int i = 0; i < len; ++i) {
        colors.at(pos + i) = color;
      }

      pos += len;
      str = match.suffix();
    }
  }
}


// print out the usage
static void usage(void) {
  printf("usage: cedar [-ih] [-e expression] [files] [args...]\n");
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

