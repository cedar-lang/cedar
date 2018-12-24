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


#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#include <utf8.h>
#include <cedar/runes.h>

using namespace cedar;


void runes::ingest_utf8(std::string s) {
	auto end_it = utf8::find_invalid(s.begin(), s.end());
  utf8::utf8to32(s.begin(), end_it, back_inserter(vec));
}


runes::runes() {
}


runes::runes(const char *s) {
	ingest_utf8(std::string(s));
}

runes::runes(char *s) {
	ingest_utf8(std::string(s));
}


runes::runes(const char32_t *s) {
	vec.clear();
	for (uint64_t l = 0; s[l]; l++) {
		vec.push_back(s[l]);
	}
}

runes::runes(std::string s) {
	ingest_utf8(s);
}
// copy constructor
runes::runes(const cedar::runes &other) {
	operator=(other);
	return;
}


runes& runes::operator=(const runes& o) {
	vec = o.vec;
	return *this;
}


runes::~runes() {
}


runes::iterator runes::begin(void) { return vec.begin(); }
runes::iterator runes::end(void)   { return vec.end();   }

runes::const_iterator runes::cbegin(void) const { return vec.cbegin(); }
runes::const_iterator runes::cend(void) const { return vec.cend(); }

uint32_t runes::size(void) { return vec.size(); }

uint32_t runes::length(void) { return vec.size(); }

uint32_t runes::max_size(void) { return vec.max_size(); }

uint32_t runes::capacity(void) { return vec.capacity(); }

void runes::clear(void) {
	vec.clear();
}

bool runes::empty(void) {
	return vec.empty();
}


runes& runes::operator+=(const runes& other) {
	for (auto it = other.cbegin(); it != other.cend(); it++) {
		vec.push_back(*it);
	}

	return *this;
}


runes& runes::operator+=(const char *other) {
	return operator+=(runes(other));
}


runes& runes::operator+=(std::string other) {
	return operator+=(runes(other));
}


runes& runes::operator+=(char c) {
	vec.push_back(c);
	return *this;
}

runes& runes::operator+=(rune c) {
	vec.push_back(c);
	return *this;
}

bool runes::operator==(const runes& other) {
	return other.vec == vec;
}


runes::operator std::string() const {
	std::string unicode;
  utf8::utf32to8(vec.begin(), vec.end(), back_inserter(unicode));
	return unicode;
}



rune runes::operator[](size_t i) {
	return vec[i];
}

rune runes::operator[](size_t i) const {
	if (i >= vec.size() || i < 0) {
		return 0;
	}
	return vec[i];
}

void runes::push_back(rune& r) {
	vec.push_back(r);
}

void runes::push_back(rune&& r) {
	vec.push_back(r);
}
