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


#include <cedar/runes.h>

using namespace cedar;


#define ALIGNMENT 8
// rounds up to the nearest multiple of ALIGNMENT
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


void runes::init(uint32_t size) {
	cap = ALIGN(size);
	len = 0;
	buf = new rune[cap];

	for (int i = 0; i < cap; i++) buf[i] = 0;
}

void runes::resize(uint32_t size, rune c) {
	uint32_t new_cap = ALIGN(size);

	if (new_cap > cap) {
		new_cap = ALIGN(cap * 2);

		rune *new_buf = new rune[new_cap];
		for (int i = 0; i < cap; i++)
			new_buf[i] = buf[i];

		for (int i = cap; i < new_cap; i++)
			new_buf[i] = c;

		delete buf;
		buf = new_buf;
		cap = new_cap;
	}
}


void runes::ingest_utf8(std::string s) {
	init(1);
	len = 0;

	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
	std::u32string utf32 = cvt.from_bytes(s);


	for (rune r : utf32) {
		push_back(r);
	}
}


runes::runes() {
	init();
}


runes::runes(const char *s) {
	ingest_utf8(std::string(s));
}

runes::runes(char *s) {
	ingest_utf8(std::string(s));
}


runes::runes(const char32_t *s) {
	uint32_t l;
	for (l = 0; s[l]; l++);
	init(l);
	len = l;
	for (int i = 0; i < l; i++) buf[i] = s[i];
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

	bool alloc_again = false;

	if (o.len > cap) {
		alloc_again = true;
	}


	if (alloc_again || buf == nullptr) {
		if (buf != nullptr) delete buf;
		cap = o.cap;
		buf = new rune[cap];
	}

	// update the len locally to be the other string's length
	len = o.len;
	for (int i = 0; i < cap; i++)
		buf[i] = o.buf[i];

	return *this;
}


runes::~runes() {
	delete buf;
}

rune *runes::begin(void) { return buf; }

rune *runes::end(void) {
	return buf + len;
}

rune *runes::cbegin(void) const { return buf; }

rune *runes::cend(void) const { return buf + len; }

uint32_t runes::size(void) { return len; }

uint32_t runes::length(void) { return len; }

uint32_t runes::max_size(void) { return -1; }

uint32_t runes::capacity(void) { return cap; }

void runes::clear(void) {
	for (int i = 0; i < len; i++) buf[i] = '\0';
	len = 0;
}

bool runes::empty(void) {
	return len == 0;
}


runes& runes::operator+=(const runes& other) {

	for (auto it = other.cbegin(); it != other.cend(); it++) {
		push_back(*it);
	}
	return *this;
}


runes& runes::operator+=(const char *other) {

	for (int i = 0; other[i]; i++)
		push_back(other[i]);
	return *this;
}


runes& runes::operator+=(std::string other) {
	return operator+=(runes(other));
}
runes& runes::operator+=(char c) {
	push_back(c);
	return *this;
}

runes& runes::operator+=(rune c) {
	push_back(c);
	return *this;
}

bool runes::operator==(const runes& other) {
	if (other.len != len) return false;
	// they are the same length...
	for (int i = 0; i < len; i++) {
		if (other[i] != buf[i]) return false;
	}
	return true;
}


runes::operator std::string() const {
	std::string s;

	std::u32string utf32;

	for(auto it = cbegin(); it != cend(); ++it) {
		utf32.push_back(*it);
	}

	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
	std::string utf8 = cvt.to_bytes(utf32);

	return utf8;
}


rune runes::operator[](size_t pos) const {
	return *(buf + pos);
}

void runes::push_back(rune& r) {
	if (len >= cap) {
		resize(len+1);
	}
	buf[len] = r;
	len += 1;
}

void runes::push_back(rune&& r) {
	if (len >= cap) {
		resize(len+1);
	}
	buf[len] = r;
	len += 1;
}
