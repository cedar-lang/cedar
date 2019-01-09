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


// this file includes most of the standard bindings and runtime functions required
// by the cedar standard library. They will be linked at runtime via a dlopen and dlsym
// binding defined in src/main.cpp

#include <cedar/vm/binding.h>
#include <cedar.h>

using namespace cedar;

cedar_binding(cedar_add) {

	ref accumulator = 0;

	while (true) {
		if (args.get_first().is_nil()) break;
		accumulator = accumulator + args.get_first();
		args = args.get_rest();
	}

	return accumulator;
}

cedar_binding(cedar_sub) {

	ref accumulator = 0;

	while (true) {
		if (args.get_first().is_nil()) break;
		accumulator = accumulator - args.get_first();
		args = args.get_rest();
	}

	return accumulator;
}


cedar_binding(cedar_print) {
	while (true) {
		if (args.is_nil()) break;
		std::cout << args.get_first().to_string(true) << " ";
		args = args.get_rest();
	}
	std::cout << std::endl;
	return 0;
}

cedar_binding(cedar_typeof) {
	cedar::runes kw_str = ":";
	kw_str += args.get_first().object_type_name();

	return new_obj<cedar::keyword>(kw_str);
}

