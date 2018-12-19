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

#include <cedar/parser.h>
#include <cedar/exception.hpp>
#include <cedar/runes.h>
#include <cedar/memory.h>

#include <iostream>
#include <string>
#include <cctype>


using namespace cedar;


token::token() {
	type = tok_eof;
}

token::token(uint8_t t, cedar::runes v) {
	type = t;
	val = v;
}


lexer::lexer(cedar::runes src) {
	source = src;
	index = 0;
}


cedar::rune lexer::next() {
	auto c = peek();
	index++;
	return c;
}

cedar::rune lexer::peek() {
	if (index > source.size()) {
			return -1;
	}
	return source[index];
}



bool in_charset(wchar_t c, const wchar_t *set) {
	for (int i = 0; set[i]; i++) {
		if (set[i] == c) return true;
	}

	return false;
}

token lexer::lex() {

	auto c = next();


	if (c == ';') {
		while (peek() != '\n' && peek() != -1) next();

		return lex();
	}

	// skip over spaces by calling again
	if (isspace(c))
		return lex();

	if (c == -1 || c == 0) {
		return token(tok_eof, "");
	}


	if (c == '`') {
		return token(tok_backquote, "`");
	}


	if (c == ',') {
		if (peek() == '@') {
			next();
			return token(tok_comma_at, ",@");
		}

		return token(tok_comma, ",");
	}


	if (c == '(')
		return token(tok_left_paren, "(");

	if (c == ')')
		return token(tok_right_paren, ")");


	if (c == '\'')
		return token(tok_quote, "'");


	if (c == '"') {
		cedar::runes buf;


		bool escaped = false;
		// ignore the first quote because a string shouldn't
		// contain the encapsulating quotes in it's internal representation
		while (true) {
			c = next();
			// also ignore the last double quote for the same reason as above
			if (c == '"' && !escaped) break;
			escaped = c == '\\';
			buf += c;
		}


		return token(tok_string, buf);
	}

	// parse a number
	if (isdigit(c) || c == '.') {
		// it's a number (or it should be) so we should parse it as such

		cedar::runes buf;


		buf += c;
		bool has_decimal = c == '.';

		while (isdigit(peek()) || peek() == '.') {
			c = next();
			buf += c;
			if (c == '.') {
				if (has_decimal)
					throw make_exception("invalid number syntax: '", buf, "' too many decimal points");
				has_decimal = true;
			}
		}

		if (buf == ".") {
			return token(tok_ident, buf);
		}
		return token(tok_number, buf);
	} // digit parsing


	// it wasn't anything else, so it must be
	// either an ident or a keyword token


	cedar::runes ident;
	ident += c;

	while (!in_charset(peek(), L" \n\t(){},'`@") && peek() != -1) {
		ident += next();
	}


	if (ident.length() == 0)
		throw make_exception("lexer encountered zero-length identifier");

	uint8_t type = tok_ident;

	if (ident[0] == ':')
		type = tok_keyword;

	return token(type, ident);
}

// ----------- reader -----------
reader::reader() {}

std::vector<ref> reader::read_top_level_objects(cedar::runes source) {
	std::vector<ref> statements;

	lexer = make<cedar::lexer>(source);

	// reset the parser's lexer state
	tokens.empty();

	// read all the tokens from the source code
	token t;
	while ((t = lexer->lex()).type != tok_eof)
		tokens.push_back(t);
	// reset internal state of the reader
	tok = tokens[0];
	index = 0;

	while (tok.type != tok_eof) {
		
	}

	// delete the lexer state
	lexer = nullptr;

	return statements;
}


token reader::peek(int offset) {
	int new_i = index + offset;
	if (new_i < 0 || new_i > tokens.size()) {
		return token(tok_eof, "");
	}

	return tokens[new_i];
}

token reader::move(int offset) {
	tok = peek(offset);
	index += offset;
	return tok;
}

token reader::next(void) {
	return move(1);
}

token reader::prev(void) {
	return move(-1);
}














