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

#include <cedar/object.h>
#include <cedar/object/keyword.h>
#include <cedar/memory.h>
#include <cedar/util.hpp>

using namespace cedar;

static ref the_nil = nullptr;

cedar::keyword::keyword(void) {}
cedar::keyword::keyword(cedar::runes content) {
	m_content = content;
}


cedar::keyword::~keyword() {
}

cedar::runes keyword::to_string(bool) {
	return m_content;
}

void keyword::set_content(cedar::runes content) {
	m_content = content;
}

cedar::runes keyword::get_content(void) {
	return m_content;
}


ref keyword::to_number() {
	throw cedar::make_exception("Attempt to cast keyword to number failed");
}



u64 keyword::hash(void) {
	if (!m_hash_calculated) {
		m_hash = std::hash<cedar::runes>()(m_content);
		m_hash_calculated = true;
	}
	return m_hash;
}


