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

#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/object/number.h>
#include <cedar/object/nil.h>

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
		while (peek() != '\n' && (int32_t)peek() != -1) next();

		return lex();
	}

	// skip over spaces by calling again
	if (isspace(c))
		return lex();

	if ((int32_t)c == -1 || c == 0) {
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
			return token(tok_symbol, buf);
		}
		return token(tok_number, buf);
	} // digit parsing


	// it wasn't anything else, so it must be
	// either an symbol or a keyword token


	cedar::runes symbol;
	symbol += c;

	while (!in_charset(peek(), L" \n\t(){},'`@")) {
		symbol += next();
	}


	if (symbol.length() == 0)
		throw make_exception("lexer encountered zero-length identifier");

	uint8_t type = tok_symbol;

	if (symbol[0] == ':')
		type = tok_keyword;

	return token(type, symbol);
}

// ----------- reader -----------
reader::reader() {}

std::vector<ref> reader::run(cedar::runes source) {
	std::vector<ref> statements;

	m_lexer = make<cedar::lexer>(source);

	// reset the parser's lexer state
	tokens.empty();

	// read all the tokens from the source code
	token t;
	while ((t = m_lexer->lex()).type != tok_eof) {
		tokens.push_back(t);
	}
	// reset internal state of the reader
	tok = tokens[0];
	index = 0;

	while (tok.type != tok_eof) {
		ref obj = parse_expr();
		statements.push_back(obj);
	}

	// delete the lexer state
	m_lexer = nullptr;

	return statements;
}

token reader::peek(int offset) {
	unsigned long new_i = index + offset;
	if (new_i < 0 || new_i >= tokens.size()) {
		return token(tok_eof, U"");
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



ref reader::parse_expr(void) {

	switch (tok.type) {
		case tok_string:
		case tok_keyword:
		case tok_symbol:
			return parse_symbol();
		case tok_number:
			return parse_number();
		case tok_left_paren:
			return parse_list();
		case tok_backquote:
			return parse_special_syntax(U"quasiquote");
		case tok_quote:
			return parse_special_syntax(U"quote");
		case tok_comma:
			return parse_special_syntax(U"unquote");
		case tok_comma_at:
			return parse_special_syntax(U"unquote-splicing");
	}
	throw cedar::make_exception("Unimplmented token: ", tok);
}

ref reader::parse_list(void) {
	ref obj = new_const_obj<list>();

	std::vector<ref> items;

	// skip over the first paren
	next();


	while (tok.type != tok_right_paren) {
		if (tok.type == tok_eof) {
			throw make_exception("unexpected eof in list");
		}
		ref item = parse_expr();
		items.push_back(item);
	}

	unsigned len = items.size();
	unsigned last_i = len - 1;
	ref curr = obj;
	for (unsigned i = 0; i < len; i++) {
		ref lst = new_const_obj<list>();
		curr.set_first(items[i]);

		if (i+1 <= last_i && items[i+1]->is<symbol>() && items[i+1]->as<symbol>()->get_content() == ".") {
			if (i+1 != last_i-1) throw make_exception("Illegal end of dotted list");
			curr.set_rest(items[last_i]);
			if (curr.get_rest()->is<nil>()) {
				curr.set_rest(new_const_obj<list>());
			}
			break;
		}

		curr.set_rest(lst);
		curr = lst;
	}


	// skip over the closing paren
	next();

	return obj;
}




ref reader::parse_special_syntax(cedar::runes function_name) {
	ref obj = new_const_obj<list>();
	obj.set_first(new_const_obj<symbol>(function_name));
	next();
	obj.set_rest(new_const_obj<list>());
	ref val = parse_expr();
	// print(obj.get_rest());
	// obj.get_rest().set_first(val);
	// print(obj);
	return obj;
}



ref reader::parse_symbol(void) {
	ref return_obj;

	// since parse_symbol also parses keywords, the parser
	// needs to special case symbols that start with ':'
	if (tok.val[0] == ':') {
		throw cedar::make_exception("Keyword generation unimplemented, plz fix");
	} else if (tok.val == "nil") {
		return_obj = get_nil();
	} else {
		return_obj = new_const_obj<cedar::symbol>(tok.val);
	}
	next();
	return return_obj;
}




ref reader::parse_number(void) {
	std::string str = tok.val;
	double value = atof(str.c_str());
	ref obj = new_const_obj<number>(value);
	next();
	return obj;
}



