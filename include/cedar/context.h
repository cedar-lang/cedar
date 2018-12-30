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

#include <cedar/parser.h>
#include <cedar/vm/machine.h>
#include <cedar/memory.h>

#include <vector>
#include <mutex>

namespace cedar {

	/*
	 * context
	 *
	 * the base form of state in a cedar VM instance.
	 * it contains methods for parsing, optimizing, compiling,
	 * and evaluating files, strings, and other forms of data.
	 */
	class context {
		private:
			std::shared_ptr<cedar::reader> reader;
			// require a lock for reading
			std::mutex parse_lock;

			// the bytecode for this object
			std::vector<uint8_t> code;


			std::shared_ptr<cedar::vm::machine> m_evaluator = nullptr;

		public:
			context();
			void eval_file(cedar::runes);
			void eval_string(cedar::runes);
	};
}
