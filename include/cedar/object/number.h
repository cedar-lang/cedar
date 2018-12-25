
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
#include <cedar/ref.hpp>

namespace cedar {
	// number is an in-language object representation of a number type
	// and it's the only kind of type that can be used in calculations
	//
	// It will possibly allow different types of numbers (double, int, etc...)
	// but for now it just is a 64 bit double precsion floating point number
	class number : public object {
		private:
			// a list stores two references, the car/cdr in old lisp
			// they will default to the `nil` singleton object
			double m_val = 0.0;

		public:
			number(void);
			number(double);
			~number(void);

			cedar::runes to_string(bool human = false);
			ref to_number();
	};
}
