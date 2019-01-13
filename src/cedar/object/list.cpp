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
	int len = items.size();
	ref curr = nullptr;
	for (int i = len-1; i >= 0; i--) {
		ref lst = new_obj<list>();
		ref item = items[i];
		lst.set_first(item);

		if (i-1 >= 0) {
			if (auto *sym = ref_cast<symbol>(items[i-1]); sym != nullptr) {
				if (sym->get_content() == ".") {
					if (i == len - 1 && len >= 3) {
						lst.set_rest(item);
						i--;
						lst.set_first(items[i-1]);
						i--;
						curr = lst;
						continue;
					} else {
						throw cedar::make_exception("illegal end of dotted list");
					}
				}
			}
		}

		lst.set_rest(curr);
		curr = lst;
	}

	set_first(curr.get_first());
	set_rest(curr.get_rest());
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
		if (m_first.is_nil()) return "(nil)";
		s += "(";
		s += m_first.to_string();
		s += ")";
		return s;
	}

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

		if (curr.get_rest().is_nil()) {
			break;
		} else {
			s += " ";
		}

		if (!curr.get_rest().is_nil()) {
			if (curr.get_rest()->is_pair()) {
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

/*
static Py_hash_t
tuplehash(PyTupleObject *v)
{
    Py_uhash_t x;
    Py_hash_t y;
    Py_ssize_t len = Py_SIZE(v);
    PyObject **p;
    Py_uhash_t mult = _PyHASH_MULTIPLIER;
    x = 0x345678UL;
    p = v->ob_item;
    while (--len >= 0) {
        y = PyObject_Hash(*p++);
        if (y == -1)
            return -1;
        x = (x ^ y) * mult;
        mult += (Py_hash_t)(82520UL + len + len);
    }
    x += 97531UL;
    if (x == (Py_uhash_t)-1)
        x = -2;
    return x;
}
*/

u64 list::hash(void) {

	u64 x = 0;
	u64 y = 0;

	i64 mult = 1000003UL; // prime multiplier

	x = 0x345678UL;


	y = m_first.hash();
	x = (x ^ y) * mult;
	mult += (u64)(852520UL + 2);

	y = m_rest.hash();
	x = (x ^ y) * mult;
	mult += (u64)(852520UL);
	x += 97531UL;
	return x;
	/*
	u64 first_hash = m_first.hash();
	u64 rest_hash = m_rest.hash();
	first_hash = ((first_hash & TOP_MASK) ^ ((first_hash & BOTTOM_MASK) << 32));
	rest_hash = ((rest_hash & BOTTOM_MASK) ^ ((rest_hash & TOP_MASK) >> 32));
	return first_hash ^ rest_hash;
	*/
}
