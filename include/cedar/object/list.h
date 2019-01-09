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

	// the `list` type is the most common sequence used in cedar
	// it is what gets read by the reader when the sequence (..)
	// is encountered.
	//
	// It's also known as `cons` in some lisps, and you use `cons`
	// to construct a lisp
	class list : public sequence {
		private:
			// a list stores two references, the car/cdr in old lisp
			// they will default to the `nil` singleton object
			ref m_first = nullptr;
			ref m_rest = nullptr;

		public:
			list(void);
			list(ref, ref);
			list(std::vector<ref>);
			~list(void);
			ref get_first(void);
			ref get_rest(void);

			void set_first(ref);
			void set_rest(ref);

			ref to_number();

			inline const char *object_type_name(void) {
				if (m_rest.is_nil() && m_first.is_nil())
					return "nil";
				return "list";
			};
		protected:
			cedar::runes to_string(bool human = false);
	};
}
