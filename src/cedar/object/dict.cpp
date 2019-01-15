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
#include <cedar/object/dict.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/memory.h>
#include <cedar/util.hpp>

using namespace cedar;

cedar::dict::dict(void) {
}

cedar::dict::~dict(void) {
}

ref dict::to_number(void) {
	throw cedar::make_exception("Attempt to cast dict to number failed");
}

cedar::runes dict::to_string(bool human) {
	cedar::runes str = "(dict";
	for (auto it : table) {
		str += " ";
		ref key = const_cast<cedar::ref&>(it.first);
		if (key.is<list>() || key.is<symbol>()) str += "'";
		str += key.to_string();
		str += " ";
		str += const_cast<cedar::ref&>(it.second).to_string();
	}
	str += ")";
	return str;
}

ref dict::get(ref k) {
	return table.at(k);
}


void dict::set(ref k, ref v) {
	table[k] = v;
}

u64 dict::hash(void) {

	u64 x = 0;
	u64 y = 0;

	i64 mult = 1000003UL; // prime multiplier

	x = 0x345678UL;

	for (auto it : table) {
		y = const_cast<ref&>(it.first).hash();
		x = (x ^ y) * mult;
		mult += (u64)(852520UL + 2);

		y = const_cast<ref&>(it.second).hash();
		x = (x ^ y) * mult;
		mult += (u64)(852520UL);
		x += 97531UL;
	}
	return x;
}


ref dict::keys(void) {
	ref l = new_obj<list>();

	for (auto it : table) {
		l.set_first(const_cast<ref&>(it.first));
		auto n = new_obj<list>();
		n.set_rest(l);
		l = n;
	}
	return l.get_rest();
}


void cedar::dict_set(ref & d, ref k, ref v) {
	if (auto *di = ref_cast<dict>(d); di != nullptr) {
		di->set(k,v);
		return;
	}
	throw cedar::make_exception("unable to set key on non-dict");
}

ref cedar::dict_get(ref & d, ref k) {
	if (auto *di = ref_cast<dict>(d); di != nullptr) {
		return di->get(k);
	}
	throw cedar::make_exception("unable to get key on non-dict");
}
