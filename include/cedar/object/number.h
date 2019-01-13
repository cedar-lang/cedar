
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

#include <cedar/object/sequence.h>
#include <cedar/runes.h>
#include <cedar/ref.h>

namespace cedar {

	// number is a special case object. It isn't
	// meant to be constructed. as all numbers
	// simply live inside a reference and all
	// arithmatic is delegated to references
	// attempting to construct a number class
	// will result in an exception.
	//
	// this class simply exists to allow type checking
	class number : public object {
		public:
			number(void);
			number(double);
			~number(void);

			ref to_number();
			inline const char *object_type_name(void) { return "number"; };
			u64 hash(void);
		protected:
			cedar::runes to_string(bool human = false);
	};

}
