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

#include <cstdio>

#include <string>
#include <functional>
#include <cedar/ref.h>
#include <cedar/object.h>
#include <cedar/object/bytes.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/memory.h>
#include <cedar/util.hpp>


using namespace cedar;


cedar::bytes::bytes(immer::flex_vector<unsigned char> v) {
  buffer = v;
}

cedar::bytes::bytes(void) {
}

cedar::bytes::~bytes(void) {
}

ref bytes::at(int i) {
  return (i64)const_cast<unsigned char&>(buffer[i]);
}

cedar::runes bytes::to_string(bool human) {
	cedar::runes str = "#b[";
  char buf[8];
  for (u32 i = 0; i < buffer.size(); i++) {

    sprintf(buf, "0x%02x", buffer[i]);
    str += buf;
    if (i != buffer.size()-1) str += " ";
  }
	str += "]";
	return str;
}

ref bytes::get(ref k) {
  i64 i = k.to_int();
  if (i < 0 || i >= (i64)buffer.size()) return nullptr;
	return at(i);
}


ref bytes::set(ref k, ref v) {
  i64 i = k.to_int();
  if (i < 0 || i >= (i64)buffer.size())
    throw cedar::make_exception("bytes set out of bounds");

  if (!v.is_int()) {
    throw cedar::make_exception("setting byte value requires an int");
  }
  return new bytes(buffer.set(i, v.to_int()));
}

ref bytes::append(ref v) {
  if (!v.is_int()) {
    throw cedar::make_exception("appending byte value requires an int");
  }
  const auto v2 = buffer.push_back(v.to_int());
  return new bytes(v2);
}

u64 bytes::hash(void) {

	u64 x = 0;
	u64 y = 0;

	i64 mult = 1000003UL; // prime multiplier

	x = 0x345678UL;

	for (auto it : buffer) {
		y = const_cast<unsigned char&>(it);
		x = (x ^ y) * mult;
		mult += (u64)(852520UL + 2);
	}
	return x;
}


ref bytes::first(void) {
  if (buffer.size() == 0) return nullptr;
  return at(0);
}

ref bytes::rest(void) {
  if (buffer.size() <= 1) return nullptr;
  return new bytes(buffer.drop(1));
}

ref bytes::cons(ref v) {
  if (!v.is_int()) {
    throw cedar::make_exception("appending byte value requires an int");
  }
  return new bytes(buffer.push_back(v.to_int()));
}
