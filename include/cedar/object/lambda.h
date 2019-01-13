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

#pragma once

#include <cedar/object.h>
#include <cedar/runes.h>
#include <cedar/ref.h>
#include <cedar/vm/bytecode.h>
#include <cedar/vm/machine.h>

#include <cedar/vm/binding.h>
#include <functional>
#include <memory>


// define the number of references to store in the
// closure itself, instead of in the vector
#define CLOSURE_INTERNAL_SIZE 4

namespace cedar {



	// closure represents a wraper around closed values
	// in functions, also known as "freevars" in LC
	// It will attempt to store things in a fixed sized
	// array if it can, but will also allocate a vector
	// if needed
	class closure {

		bool use_vec = false;
		std::vector<ref> vars;
		ref var_a[CLOSURE_INTERNAL_SIZE];
		public:
			// constructor to allocate n vars of closure space
			closure(int);
			~closure(void);

			ref & at(int);
	};

	class lambda : public object {
		public:

			enum lambda_type {
				bytecode_type,
				function_binding_type,
			};

			lambda_type type = bytecode_type;
			std::shared_ptr<cedar::vm::bytecode> code;
			int closure_size = -1;
			std::shared_ptr<closure> closure = nullptr;

			bound_function function_binding;

			lambda(void);
			lambda(std::shared_ptr<cedar::vm::bytecode>);
			lambda(cedar::bound_function);
			~lambda(void);

			ref to_number();
			inline const char *object_type_name(void) { return "lambda"; };
			u64 hash(void);

		protected:
			cedar::runes to_string(bool human = false);
	};
}
