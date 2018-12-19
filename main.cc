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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <cedar.h>
#include <getopt.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>


#include <cedar/object/list.h>

void usage(void) {
	printf("usage: cedar [-i] [-e expression] [files...]\n");
}

/*
 * this file is the entry point for the `cedar` command line utility, as cedar
 * is a library that can be dynamically linked into any c++ program. Because of
 * this, the primary cedar program is dynamically linked to libcedar.so, wherever
 * that might be.
 */


int main(int argc, char **argv) {

	cedar::ref l = cedar::new_obj<cedar::list>();

	l.first() = cedar::new_obj<cedar::number>(30);
	l.rest() = cedar::new_obj<cedar::string>("hello there");

	cedar::print(l, l.first(), l.rest());

	// src is the program that will be evaulated
	// where it is loaded in is determined later
	cedar::runes src;

	if (argc > 1) {
		src = cedar::util::read_file(argv[1]);
	}

	try {

		auto ctx = cedar::make<cedar::context>();
		// auto r = cedar::make<cedar::reader>(src);
		bool interactive = false;

		char c;
		while ((c = getopt(argc, argv, "ie:")) != -1) {

			switch (c) {
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
	}	catch (std::exception & e) {
		std::cerr << "Fatal Exception: " << e.what() << std::endl;
		exit(-1);
	}

	return 0;
}
