// (c) 2018 Nick Wanninger
// This code is licensed under MIT license (see LICENSE for details)

#include <cedar/parser.hh>
#include <cedar/exception.hpp>
#include <cedar.hh>

#include <iostream>
#include <string>
#include <cctype>


using namespace cedar;


token::token() {
	type = tok_eof;
}

token::token(uint8_t t, std::wstring v) {
	type = t;
	val = v;
}


lexer::lexer(std::wstring src) {
	source = src;
	index = 0;
}


wchar_t lexer::next() {
	auto c = peek();
	index++;
	return c;
}

wchar_t lexer::peek() {
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

	wchar_t c = next();


	if (c == ';') {
		while (peek() != '\n' && peek() != -1) next();

		return lex();
	}

	// skip over spaces by calling again
	if (isspace(c))
		return lex();

	if (c == -1 || c == 0) {
		return token(tok_eof, L"");
	}


	if (c == '`') {
		return token(tok_backquote, L"`");
	}


	if (c == ',') {
		if (peek() == '@') {
			next();
			return token(tok_comma_at, L",@");
		}

		return token(tok_comma, L",");
	}


	if (c == '(')
		return token(tok_left_paren, L"(");

	if (c == ')')
		return token(tok_right_paren, L")");


	if (c == '\'')
		return token(tok_quote, L"'");

	// parse a number
	if (isdigit(c) || c == '.') {
		// it's a number (or it should be) so we should parse it as such
		std::wostringstream buf;


		buf << c;
		bool has_decimal = c == '.';

		while (isdigit(peek()) || peek() == '.') {
			c = next();
			buf << c;
			if (c == '.') {
				if (has_decimal)
					throw make_exception("invalid number syntax: '", buf.str(), "' too many decimal points");
				has_decimal = true;
			}
		}

		std::wstring val = buf.str();
		if (val == L".") {
			return token(tok_ident, val);
		}
		return token(tok_number, val);
	} // digit parsing


	// it wasn't anything else, so it must be
	// either an ident or a keyword token


	std::wstring ident;
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


reader::reader(std::wstring source) {
	lexer = std::make_shared<cedar::lexer>(source);
}

token reader::lex() {
	return lexer->lex();
}


ptr<cedar::object> reader::run(void) {
	while (true) {
		const token t = lex();
		if (t.type == tok_eof) break;

		std::cout << t << std::endl;

	}
	return std::make_shared<cedar::object>();
}
