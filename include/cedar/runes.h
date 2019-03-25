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

#include <string>

namespace cedar {

	using rune = uint32_t;

	class runes {
		public:

		  // using container = std::u32string;
		  using container = std::u32string;

		protected:

			friend std::hash<cedar::runes>;

			container buf;

			void ingest_utf8(std::string const&);

		public:

			typedef container::iterator iterator;
			typedef container::const_iterator const_iterator;

			typedef rune value_type;
			typedef rune& reference;
			typedef const rune& const_reference;

			runes(void);
			runes(char*);
			runes(const char*);
			runes(const char32_t*);
			runes(std::string const&);
			runes(std::u32string const&);
			runes(const cedar::runes&);

			~runes(void);


			iterator begin(void);
			iterator end(void);

			const_iterator cbegin(void) const;
			const_iterator cend(void) const;

			uint32_t size(void);
			uint32_t length(void);
			uint32_t max_size(void);

			void resize(uint32_t, rune c = 0);

			uint32_t capacity(void);

			void clear(void);

			bool empty(void);


			runes& operator+=(const runes&);
			runes& operator+=(const char *);
			runes& operator+=(std::string const&);
			runes& operator+=(char);
			runes& operator+=(int);
			runes& operator+=(rune);

			runes& operator=(const runes&);

			bool operator==(const runes &other) const;

			bool operator==(const runes&);


			operator std::string() const;
			operator std::u32string() const;

			rune operator[](size_t) const;
			rune operator[](size_t);


			void push_back(rune&);
			void push_back(rune&&);
	};


	inline std::ostream& operator<<(std::ostream& os, runes& r) {
		std::string s = r;
		return os << s;
	}

	inline std::ostream& operator<<(std::ostream& os, const runes& r) {
		std::string s = r;
		return os << s;
	}

};


template <>
struct std::hash<cedar::runes> {
	std::size_t operator()(const cedar::runes& k) const
	{
		std::hash<cedar::runes::container> hash_func;
		return hash_func(k.buf);
	}
};
