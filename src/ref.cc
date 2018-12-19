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

#include <cedar/ref.hpp>
#include <cedar/object.h>
#include <cedar/object/sequence.h>

using namespace cedar;

uint16_t cedar::change_refcount(object *o, int change) {
	if (o->refcount == 0 && change < 0) return 0;
	return o->refcount += change;
}

uint16_t cedar::get_refcount(object *o) {
	return o->refcount;
}

void cedar::delete_object(object *o) {
	if (!o->no_autofree)
		delete o;
}


ref& cedar::ref::first() const {
	return reinterpret_cast<sequence*>(obj)->first();
}

ref& cedar::ref::rest() const {
	return reinterpret_cast<sequence*>(obj)->rest();
}
