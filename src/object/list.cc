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


#include <cedar/object/list.h>
#include <cedar/object/nil.h>
#include <cedar/ref.hpp>

using namespace cedar;


list::list(void) {
	m_first = get_nil();
	m_rest = get_nil();
}

list::~list(void) {

}

ref list::get_first(void) {
	return this->m_first;
}


ref list::get_rest(void) {
	return this->m_rest;
}

void list::set_first(ref n_first) {
	this->m_first = n_first;
}


void list::set_rest(ref n_rest) {
	this->m_rest = n_rest;
}

cedar::runes list::to_string(bool human) {
	cedar::runes s;


	if (m_first->is<nil>()) {
		return "()";
	}

	if (is_pair()) {
		s += "(";
		s += m_first->to_string();
		s += " . ";
		s += m_rest->to_string();
		s += ")";
		return s;
	}


	s += "(";
	ref curr = this;

	while (!curr.get_first()->is<nil>()) {

		s += curr.get_first()->to_string();
		if (!curr.get_rest()->is<nil>()) {
			if (curr.get_rest()->is_pair()) {
				s += " ";
				s += curr.get_rest().get_first()->to_string();
				s += " . ";
				s += curr.get_rest().get_rest()->to_string();
				break;
			}
			if (!curr.get_rest().get_first()->is<nil>()) {
				s += " ";
			}
			curr = curr.get_rest();
		}
	}
	s += ")";

	return s;
}

ref list::to_number() {
	throw cedar::make_exception("Attempt to cast list to number failed");
}
