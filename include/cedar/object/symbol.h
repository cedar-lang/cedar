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

namespace cedar {


	class symbol : public object {
		private:
		public:
      i32 id;
			symbol(void);
			symbol(cedar::runes);
			~symbol(void);

			void set_content(cedar::runes);
			cedar::runes get_content(void);

			inline const char *object_type_name(void) { return "symbol"; };
			u64 hash(void);

		protected:
			cedar::runes to_string(bool human = false);
	};

  i32 get_symbol_intern_id(cedar::runes);

  inline ref newsymbol(cedar::runes r) {
    return new symbol(r);
  }

  cedar::runes get_symbol_id_runes(int);
}
