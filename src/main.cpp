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
#include <thread>
#include <unistd.h>

struct cedar_options {
	bool interactive = false;
};

void cedar_start(cedar_options &);


static void usage(void);
static void help(void);

void cedar_start(cedar_options &) {
	auto eval_thread = std::thread();
}


int main(int argc, char** argv) {

	srand((unsigned int)time(nullptr));


	try {
		auto ctx = std::make_shared<cedar::context>();
		char c;
		while ((c = getopt(argc, argv, "ihe:")) != -1) {
			switch (c) {
				case 'h':
					help();
					exit(0);
				case 'i':
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

	} catch (std::exception& e) {
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




