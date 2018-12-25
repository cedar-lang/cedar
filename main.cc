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

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <string>

#include <cedar.h>
#include <cedar/opcode.h>
#include <cedar/vm/bytecode.h>


static void usage(void);
static void help(void);

int main(int argc, char** argv) {


	cedar::print(sizeof(cedar::number));

  srand((unsigned int)time(nullptr));

  // src is the program that will be evaulated
  // where it is loaded in is determined later
  cedar::runes src;

  if (argc > 1) {
    src = cedar::util::read_file(argv[1]);
  }

  try {
    auto ctx = cedar::make<cedar::context>();
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
	  ctx->eval_expr(expr);
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


	} catch (const utf8::invalid_code_point& e) {
		fprintf(stderr, "\n\n\nunknown code point (%U): %s\n\n\n\n", e.code_point(), e.what());
		exit(-1);
	} catch (const std::length_error& le) {
		std::cerr << "Length error: " << le.what() << '\n';
  } catch (const std::out_of_range& oor) {
    std::cerr << "Out of Range error: " << oor.what() << '\n';
  } catch (std::exception& e) {

		std::cerr << "Exception name: " << typeid(e).name() << std::endl;
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


void *operator new(size_t size) {
	return calloc(size, 1);
}

void operator delete(void *ptr) noexcept {
	free(ptr);
}
