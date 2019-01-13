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


// this file includes most of the standard bindings and runtime functions required
// by the cedar standard library. They will be linked at runtime via a dlopen and dlsym
// binding defined in src/main.cpp

#include <cedar/vm/binding.h>
#include <cedar.h>

using namespace cedar;

cedar_binding(cedar_hash) {
	// TODO: add longs in the lang
	assert(sizeof(double) == sizeof(u64));
	u64 h = args.get_first().hash();
	return ref((i64)h);
}

cedar_binding(cedar_add) {

	ref accumulator = 0;
	while (true) {
		accumulator = accumulator + args.get_first();
		if (args.get_rest().is_nil()) break;
		args = args.get_rest();
	}
	return accumulator;
}

cedar_binding(cedar_sub) {
	ref acc = args.get_first();
	args = args.get_rest();
	int i = 0;
	while (true) {
		i++;
		acc = acc - args.get_first();

		if (args.get_rest().is_nil()) break;
		args = args.get_rest();
	}
	if (i == 0) {
		acc = acc * -1;
	}
	return acc;
}

cedar_binding(cedar_mul) {

	ref accumulator = 1;

	while (true) {
		accumulator = accumulator * args.get_first();
		if (args.get_rest().is_nil()) break;
		args = args.get_rest();
	}

	return accumulator;
}

cedar_binding(cedar_div) {
	ref acc = args.get_first();
	args = args.get_rest();
	int i = 0;
	while (true) {
		if (args.is_nil()) break;
		i++;
		acc = acc / args.get_first();
		args = args.get_rest();
	}
	if (i == 0) {
		acc = ref{1} / acc;
	}
	return acc;
}



cedar_binding(cedar_equal) {
	ref first = args.get_first();
	args = args.get_rest();
	while (true) {
		if (args.get_first() != first) return nullptr;

		if (args.get_rest().is_nil()) break;
		args = args.get_rest();
	}

	return cedar::new_obj<cedar::symbol>("t");
}


cedar_binding(cedar_print) {
	while (true) {
		std::cout << args.get_first().to_string(true) << " ";
		if (args.get_rest().is_nil()) break;
		args = args.get_rest();
	}
	std::cout << std::endl;
	return 0;
}

cedar_binding(cedar_typeof) {
	cedar::runes kw_str = ":";
	kw_str += args.get_first().object_type_name();

	return new_obj<cedar::keyword>(kw_str);
}

cedar_binding(cedar_first) {
	ref arg = args.get_first();
	return arg.get_first();
}

cedar_binding(cedar_rest) {
	return args.get_first().get_rest();
}


cedar_binding(cedar_pair) {
	ref l = new_obj<cedar::list>();
	l.set_first(args.get_first());
	l.set_rest(args.get_rest().get_first());
	return l;
}



cedar_binding(cedar_newdict) {
	ref d = cedar::new_obj<cedar::dict>();

	while (!args.is_nil()) {
		auto key = args.get_first();
		args = args.get_rest();
		auto val = args.get_first();
		args = args.get_rest();
		dict_set(d, key, val);
	}
	return d;
}

cedar_binding(cedar_dict_get) {
	ref d = args.get_first();
	args = args.get_rest();

	ref k = args.get_first();
	args = args.get_rest();

	return dict_get(d, k);
}
cedar_binding(cedar_dict_set) {
	ref d = args.get_first();
	args = args.get_rest();

	ref k = args.get_first();
	args = args.get_rest();

	ref v = args.get_first();
	args = args.get_rest();

	dict_set(d, k, v);
	return v;
}

cedar_binding(cedar_dict_keys) {
	ref d = args.get_first();
	if (auto *dp = ref_cast<dict>(d); dp != nullptr) {
		return dp->keys();
	}
	throw cedar::make_exception("(keys) call failed on non-dict");
}

void init_binding(cedar::vm::machine *m) {
	m->bind("dict", cedar_newdict);
	m->bind("get", cedar_dict_get);
	m->bind("set", cedar_dict_set);
	m->bind("keys", cedar_dict_keys);


	m->bind("+", cedar_add);
	m->bind("-", cedar_sub);
	m->bind("*", cedar_mul);
	m->bind("/", cedar_div);

	m->bind("=", cedar_equal);
	m->bind("eq", cedar_equal);



	m->bind("type-of", cedar_typeof);
	m->bind("print", cedar_print);
	m->bind("hash", cedar_hash);

	m->bind("first", cedar_first);
	m->bind("rest", cedar_rest);

	m->bind("pair", cedar_pair);
}
