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

#include <cedar/object.h>
#include <cedar/object/nil.h>
#include <cedar/memory.h>


using namespace cedar;

static ref the_nil = nullptr;

object *cedar::get_nil_object(void) {
	if (the_nil == nullptr) {
		the_nil = new_obj<nil>();
		the_nil->no_autofree = 1;
	}
	return the_nil.get<cedar::nil>();
}

ref cedar::get_nil(void) {
	if (the_nil == nullptr) {
		the_nil = new_obj<nil>();
		the_nil->no_autofree = 1;
	}

	return the_nil;
}


cedar::nil::nil() {}
cedar::nil::~nil() {}

cedar::runes nil::to_string(bool) {
	return U"nil";
}


ref nil::to_number() {
	throw cedar::make_exception("Attempt to cast nil to number failed");
}


u64 nil::hash(void) {
	return 0lu;
}
