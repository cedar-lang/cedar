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

#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include <codecvt>
// #include <cedar::>


namespace cedar {

	class exception : public std::exception {
		private:
			std::string _what = NULL;

		public:
			exception(std::string what_arg) : _what(what_arg) {}

			const char *what() const noexcept {
				return (const char*)_what.c_str();
			};


			template<typename... Args>
				exception(Args const&... args) {
					std::ostringstream stream;
					using List = int[];
					(void)List{0, ( (void)(stream << args), 0 ) ... };
					_what = stream.str();
				}
	};


	template<typename... Args>
		inline cedar::exception make_exception(Args const&... args) {
			std::ostringstream stream;
			int a[] = {0, ( (void)(stream << args), 0 ) ... };

			return cedar::exception(stream.str());
		}

}

