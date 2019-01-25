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
#include <cedar/types.h>

namespace cedar {

	// nil is the value to represent "nothing" in cedar.
	// It has no methods, but to the user it acts and is equal
	// to an empty list. It is the sential value to represent
	// that the rest of a list is the end.
	class nil : public object {
		public:
			nil(void);
			~nil(void);
			inline const char *object_type_name(void) { return "nil"; };
			u64 hash(void);
		protected:
			cedar::runes to_string(bool human = false);
	};


	// defined in src/object/nil.cc
	//
	// get_nil() is the method that gets the singleton nil value
	// in cedar. This is because nil is used so often that
	ref get_nil(void);

	object *get_nil_object(void);
}
