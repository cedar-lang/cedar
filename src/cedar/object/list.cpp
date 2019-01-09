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
#include <cedar/object/symbol.h>
#include <cedar/ref.h>

using namespace cedar;


list::list(void) {
	m_first = nullptr;
	m_rest = nullptr;
}

list::list(ref first, ref rest) {
	m_first = first;
	m_rest = rest;
}

list::list(std::vector<ref> items) {

	ref sac = new_obj<list>();
	unsigned len = items.size();
	unsigned last_i = len - 1;
	ref curr = sac;
	for (unsigned i = 0; i < len; i++) {
		ref lst = new_const_obj<list>();
		curr.set_first(items[i]);

		if (i+1 <= last_i && items[i+1].is<symbol>() && items[i+1].as<symbol>()->get_content() == ".") {
			if (i+1 != last_i-1) throw make_exception("Illegal end of dotted list");
			curr.set_rest(items[last_i]);
			if (curr.get_rest().is_nil()) {
				curr.set_rest(new_const_obj<list>());
			}
			break;
		}

		curr.set_rest(lst);
		curr = lst;
	}

	set_first(sac.get_first());
	set_rest(sac.get_rest());
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

cedar::runes list::to_string(bool) {
	cedar::runes s;



	if (m_rest.is_nil()) {
		if (m_first.is_nil()) return "()";
		s += "(";
		s += m_first.to_string();
		s += ")";
		return s;
	}


	std::cout << new_obj<cedar::list>().is_nil() << std::endl;

	if (is_pair()) {
		s += "(";
		s += m_first.to_string();
		s += " . ";
		s += m_rest.to_string();
		s += ")";
		return s;
	}

	s += "(";
	ref curr = this;

	while (true) {
		s += curr.get_first().to_string();

		std::cout << '"' << s << '"' << std::endl;

		std::cout << curr.get_rest().is_nil() << std::endl;
		if (curr.get_rest().is_nil()) {
			break;
		} else {
			s += " ";
		}

		if (!curr.get_rest().is_nil()) {
			if (false && curr.get_rest()->is_pair()) {
				s += " ";
				s += curr.get_rest().get_first().to_string();
				s += " . ";
				s += curr.get_rest().get_rest().to_string();
				break;
			}
		}

		curr = curr.get_rest();
	}

	s += ")";

	return s;
}

ref list::to_number() {
	throw cedar::make_exception("Attempt to cast list to number failed");
}




