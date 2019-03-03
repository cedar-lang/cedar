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


#include <cedar/runes.h>
#include <utf8.h>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>



using namespace cedar;

static std::string to_utf8(std::u32string const& s) {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.to_bytes(s);
}

static std::u32string to_utf32(std::string const & s) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	return conv.from_bytes(s);
}


void runes::ingest_utf8(std::string s) { buf = to_utf32(s); }


runes::runes() {}


runes::runes(const char* s) { ingest_utf8(std::string(s)); }

runes::runes(char* s) { ingest_utf8(std::string(s)); }


runes::runes(const char32_t* s) {
  buf.clear();
  for (uint64_t l = 0; s[l] != '\0'; l++) {
    buf.push_back(s[l]);
  }
}

runes::runes(std::string s) { ingest_utf8(s); }
// copy constructor
runes::runes(const cedar::runes& other) {
  operator=(other);
  return;
}


runes::runes(std::u32string s) { buf = s; }


runes& runes::operator=(const runes& o) {
  buf = o.buf;
  return *this;
}


runes::~runes() {}


runes::iterator runes::begin(void) { return buf.begin(); }
runes::iterator runes::end(void) { return buf.end(); }

runes::const_iterator runes::cbegin(void) const { return buf.cbegin(); }
runes::const_iterator runes::cend(void) const { return buf.cend(); }

uint32_t runes::size(void) { return buf.size(); }

uint32_t runes::length(void) { return buf.size(); }

uint32_t runes::max_size(void) { return buf.max_size(); }

uint32_t runes::capacity(void) { return buf.capacity(); }

void runes::clear(void) { buf.clear(); }

bool runes::empty(void) { return buf.empty(); }


runes& runes::operator+=(const runes& other) {
  for (auto it = other.cbegin(); it != other.cend(); it++) {
    buf.push_back(*it);
  }
  return *this;
}


runes& runes::operator+=(const char* other) { return operator+=(runes(other)); }


runes& runes::operator+=(std::string other) { return operator+=(runes(other)); }


runes& runes::operator+=(char c) {
  buf.push_back(c);
  return *this;
}
runes& runes::operator+=(int c) {
  buf.push_back(c);
  return *this;
}

runes& runes::operator+=(rune c) {
  buf.push_back(c);
  return *this;
}

bool runes::operator==(const runes& other) { return other.buf == buf; }
bool runes::operator==(const runes& other) const { return other.buf == buf; }


runes::operator std::string() const { return to_utf8(buf); }
runes::operator std::u32string() const { return buf; }

rune runes::operator[](size_t i) { return buf[i]; }

rune runes::operator[](size_t i) const { return buf[i]; }

void runes::push_back(rune& r) { buf.push_back(r); }

void runes::push_back(rune&& r) { buf.push_back(r); }
