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

#include <cedar/util.hpp>
#include <cedar/object.h>
#include <cedar/memory.h>
#include <cedar/runes.h>
#include <cedar/ref.h>

#include <vector>

// allow programatic access to defining token
// types, but by only actually defining them once
#define FOREACH_TOKEN_TYPE(V) \
	V(tok_eof, 0)               \
	V(tok_number, 1)            \
	V(tok_string, 2)            \
	V(tok_right_paren, 3)       \
	V(tok_left_paren, 4)        \
	V(tok_symbol, 6) \
	V(tok_keyword, 7) \
	V(tok_quote, 8) \
	V(tok_comma, 9) \
	V(tok_comma_at, 10) \
	V(tok_backquote, 11) \



namespace cedar {


	using objref = ptr<object>;


	enum tok_type {
		#define V(name, code) name = code,
			FOREACH_TOKEN_TYPE(V)
		#undef V
	};

	// a token represents a single atomic lexeme that gets parsed
	// out of a string with cedar::parser::lexer
	class token {
		public:
			int8_t type;
			cedar::runes val;
			token();
			token(uint8_t, cedar::runes);
	};

	inline std::ostream& operator<<(std::ostream& os, const token &tok) {

		static const char *tok_names[] = {
		#define V(name, code) #name,
			FOREACH_TOKEN_TYPE(V)
		#undef V
		};

		cedar::runes buf;

		buf += '(';
		buf += tok_names[tok.type];
		buf += ", ";
		buf += '\'';
		buf += tok.val;
		buf += "')";

		os << buf;

    return os;
	}

	// a lexer takes in a unicode string and will allow you to
	// call .lex() that returns a new token for each token in a stream
	class lexer {
		private:
			uint64_t index = 0;
			cedar::runes source;
			token lex_num();
			cedar::rune next();
			cedar::rune peek();
		public:
			explicit lexer(cedar::runes);
			token lex();
	};




	class reader {
		private:
			ptr<lexer> m_lexer;

			std::vector<token> tokens;
			token tok;
			uint64_t index = 0;

			token peek(int);
			token move(int);

			token next(void);
			token prev(void);
		public:
			explicit reader();

			// simply read the top level expressions until a
			// tok_eof is encountered, returning the list
			std::vector<ref> run(cedar::runes);
			// the main switch parsing function. Callable and
			// will parse any object, delegating to other functions
			ref parse_expr(void);
			// parse list literals
			ref parse_list(void);
			// parse things like
			//    'a  -> (quote a)
			//    `a  -> (quasiquote a)
			//    `@a -> (quasiunquote a)
			ref parse_special_syntax(cedar::runes);

			// parse symbols (variable names)
			// and keyword variants
			ref parse_symbol(void);
			ref parse_string(void);

			ref parse_number(void);

	};

} // namespace cedar

