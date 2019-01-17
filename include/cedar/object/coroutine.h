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
#include <cedar/object/lambda.h>

namespace cedar {

	class coroutine : public object {
		public:

      std::vector<ref> stack;
      // stack pointer is the next location to write to on the stack
      i64 sp = 0;

      // the frame pointer of the coroutine. this points
      // to the current lambda on the stack
      i64 fp = 0;

      // the instruction pointer of the coroutine
      i8 *ip = 0;

			coroutine(void);
			coroutine(cedar::runes);
			~coroutine(void);

      i64 push(ref);
      ref pop(void);

      void push_frame(ref);
      void pop_frame(void);

      lambda *program(void);

			ref to_number();
			inline const char *object_type_name(void) { return "coroutine"; };


			u64 hash(void);

		protected:
			cedar::runes to_string(bool human = false);
	};
}
