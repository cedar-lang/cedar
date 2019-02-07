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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cedar/memory.h>
#include <cedar/parser.h>
#include <cedar/runes.h>
#include <cedar/exception.hpp>

#include <cedar/object/keyword.h>
#include <cedar/object/list.h>
#include <cedar/object/vector.h>
#include <cedar/object/nil.h>
#include <cedar/object/number.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>

#include <cctype>
#include <iostream>
#include <string>

using namespace cedar;

token::token() { type = tok_eof; }

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


  auto in_set = [] (cedar::runes & set, cedar::rune c) {
    for (auto & n : set) {
      if (n == c) return true;
    }
    return false;
  };

  auto accept_run = [&] (cedar::runes set) {
    cedar::runes buf;
    while (in_set(set, peek())) {
      buf += next();
    }
    return buf;
  };

  if (c == ';') {
    while (peek() != '\n' && (int32_t)peek() != -1) next();

    return lex();
  }

  // skip over spaces by calling again
  if (isspace(c)) return lex();

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


  if (c == '#') {
    cedar::runes tok;
    tok += '#';
    if (isalpha(peek())) {
      tok += next();
    } else {
      throw cedar::make_exception("invalid hash modifier syntax: #", peek());
    }
    return token(tok_hash_modifier, tok);
  }
  if (c == '\\') return token(tok_backslash, "\\");

  if (c == '(') return token(tok_left_paren, "(");

  if (c == ')') return token(tok_right_paren, ")");

  if (c == '[') return token(tok_left_bracket, "[");

  if (c == ']') return token(tok_right_bracket, "]");

  if (c == '{') return token(tok_left_curly, "{");

  if (c == '}') return token(tok_right_curly, "}");

  if (c == '\'') return token(tok_quote, "'");

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
  if (isdigit(c) || c == '.' || (c == '-' && isdigit(peek()))) {
    // it's a number (or it should be) so we should parse it as such

    cedar::runes buf;

    buf += c;
    bool has_decimal = c == '.';


    if (peek() == 'x') {
      next();
      cedar::runes hex = accept_run("0123456789abcdefABCDEF");
      long long x;
      std::stringstream ss;
      ss << std::hex << hex;
      ss >> x;
      auto s = std::to_string(x);
      return token(tok_number, s);
    }


    if (peek() == 'o') {
      next();
      cedar::runes hex = accept_run("012345567");
      unsigned long long x;
      std::stringstream ss;
      ss << std::oct << hex;
      ss >> x;
      auto s = std::to_string(x);
      return token(tok_number, s);
    }


    while (isdigit(peek()) || peek() == '.') {
      c = next();
      buf += c;
      if (c == '.') {
        if (has_decimal) {
          return token(tok_symbol, buf);
        }
        has_decimal = true;
      }
    }

    if (buf == ".") {
      return token(tok_symbol, buf);
    }
    return token(tok_number, buf);
  }  // digit parsing

  // it wasn't anything else, so it must be
  // either an symbol or a keyword token

  cedar::runes symbol;
  symbol += c;


  while (!in_charset(peek(), L" \n\t(){}[],'`@")) {
    auto v = next();
    if ((i32)v == -1 || v == 0) break;
    symbol += v;
  }

  if (symbol.length() == 0)
    throw make_exception("lexer encountered zero-length identifier");

  uint8_t type = tok_symbol;

  if (symbol[0] == ':') {
    if (symbol.size() == 1)
      throw cedar::make_exception(
          "Keyword token must have at least one character after the ':'");
    type = tok_keyword;
  }

  return token(type, symbol);
}

// ----------- reader -----------
reader::reader() {}

void reader::lex_source(cedar::runes src) {
  m_lexer = std::make_shared<cedar::lexer>(src);
  tokens.clear();

  // read all the tokens from the source code
  token t;
  while ((t = m_lexer->lex()).type != tok_eof) {
    tokens.push_back(t);
  }
  tok = tokens[0];
  index = 0;


}

ref reader::read_one(bool *valid) {
  if (tok.type != tok_eof) {
    ref obj = parse_expr();

    *valid = false;
    return obj;
  }

  *valid = false;
  return nullptr;
}

/////////////////////////////////////////////////////
std::vector<ref> reader::run(cedar::runes source) {
  std::vector<ref> statements;

  m_lexer = std::make_shared<cedar::lexer>(source);

  tokens.clear();

  // read all the tokens from the source code
  token t;
  while ((t = m_lexer->lex()).type != tok_eof) {
    tokens.push_back(t);
  }

  if (tokens.size() == 0) return statements;
  // reset internal state of the reader
  tok = tokens[0];
  index = 0;

  while (tok.type != tok_eof) {
    ref obj = parse_expr();
    statements.push_back(obj);
  }

  // delete the lexer state
  m_lexer = nullptr;
  tokens.empty();

  return statements;
}

/////////////////////////////////////////////////////
token reader::peek(int offset) {
  uint64_t new_i = index + offset;
  if (new_i >= tokens.size()) {
    return token(tok_eof, U"");
  }

  return tokens[new_i];
}

/////////////////////////////////////////////////////
token reader::move(int offset) {
  tok = peek(offset);
  index += offset;
  return tok;
}

/////////////////////////////////////////////////////
token reader::next(void) { return move(1); }

/////////////////////////////////////////////////////
token reader::prev(void) { return move(-1); }

/////////////////////////////////////////////////////
ref reader::parse_expr(void) {
  switch (tok.type) {
    case tok_string:
      return parse_string();
    case tok_keyword:
    case tok_symbol:
      return parse_symbol();
    case tok_number:
      return parse_number();
    case tok_left_curly:
      return parse_special_grouping_as_call("Dict", tok_right_curly);
    case tok_left_bracket:
      return parse_vector();
    case tok_left_paren:
      return parse_list();
    case tok_backslash:
      return parse_backslash_lambda();
    case tok_backquote:
      return parse_special_syntax(U"quasiquote");
    case tok_quote:
      return parse_special_syntax(U"quote");
    case tok_comma:
      return parse_special_syntax(U"unquote");
    case tok_comma_at:
      return parse_special_syntax(U"unquote-splicing");
    case tok_hash_modifier:
      return parse_hash_modifier();
  }
  throw cedar::make_exception("Unimplmented token: ", tok);
}

/////////////////////////////////////////////////////
ref reader::parse_list(void) {
  std::vector<ref> items;
  // skip over the first paren
  next();
  while (tok.type != tok_right_paren) {
    if (tok.type == tok_eof) {
      throw unexpected_eof_error("unexpected eof in list");
    }
    ref item = parse_expr();
    items.push_back(item);
  }
  ref list_obj = nullptr;
  if (items.size() > 0) {
    list_obj = cedar::new_obj<list>(items);
  }
  // skip over the closing paren
  next();

  return list_obj;
}


/////////////////////////////////////////////////////
ref reader::parse_vector(void) {
  std::vector<ref> items;
  // skip over the first bracket
  next();
  while (tok.type != tok_right_bracket) {
    if (tok.type == tok_eof) {
      throw unexpected_eof_error("unexpected eof in list");
    }
    ref item = parse_expr();
    items.push_back(item);
  }

  ref vec = new vector();
  for (auto & item : items) {
    vec = vec.cons(item);
  }
  // skip over the closing bracket
  next();

  return vec;
}

/////////////////////////////////////////////////////
ref reader::parse_special_syntax(cedar::runes function_name) {
  // skip over the "special syntax token"
  next();
  std::vector<ref> items = {new_obj<symbol>(function_name), parse_expr()};
  ref obj = new_obj<list>(items);
  return obj;
}

/////////////////////////////////////////////////////
ref reader::parse_symbol(void) {
  ref return_obj;

  // since parse_symbol also parses keywords, the parser
  // needs to special case symbols that start with ':'
  if (tok.val[0] == ':') {
    return_obj = new_obj<cedar::keyword>(tok.val);
  } else if (tok.val == "nil") {
    return_obj = nullptr;
  } else {
    return_obj = new_obj<cedar::symbol>(tok.val);
  }
  next();
  return return_obj;
}

/////////////////////////////////////////////////////
ref reader::parse_number(void) {
  std::string str = tok.val;

  bool is_float = false;

  for (auto &c : tok.val) {
    if (c == '.') {
      is_float = true;
      break;
    }
  }

  next();

  if (is_float) {
    return ref(atof(str.c_str()));
  } else {
    i64 i = atoll(str.c_str());
    return ref(i);
  }
}

/////////////////////////////////////////////////////
ref reader::parse_hash_modifier(void) {
  std::string str = tok.val;

  int mod = str[1];
  switch (mod) {
    default:
      throw cedar::make_exception("invalid hash modifier: ", str);
  }
  next();
  return nullptr;
}
/////////////////////////////////////////////////////
ref reader::parse_string(void) {
  ref obj = new_obj<string>();
  obj.as<string>()->set_content(tok.val);
  next();
  return obj;
}

/////////////////////////////////////////////////////
ref reader::parse_special_grouping_as_call(cedar::runes name,
                                           tok_type closing) {
  std::vector<ref> items;
  // skip over the first grouping oper
  next();

  // push the call name to the begining of the list
  items.push_back(new_obj<symbol>(name));
  while (tok.type != closing) {
    if (tok.type == tok_eof) {
      throw unexpected_eof_error("unexpected eof in list");
    }
    ref item = parse_expr();
    items.push_back(item);
  }
  ref list_obj = nullptr;
  if (items.size() > 0) {
    list_obj = cedar::new_obj<list>(items);
  }
  // skip over the closing paren
  next();

  return list_obj;
}


/////////////////////////////////////////////////////
ref reader::parse_backslash_lambda(void) {
  std::vector<ref> items;
  // skip over the backslash
  next();

  ref args = parse_expr();

  if (!args.isa(list_type))
    args = newlist(args);

  ref body = parse_expr();

  ref func = newlist(new symbol("fn"), args, body);

  return func;
}
