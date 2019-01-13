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

#include <cedar/object.h>
#include <cedar/object/lambda.h>
#include <cedar/memory.h>
#include <cedar/vm/instruction.h>


using namespace cedar;

/////////////////////////////////////////////////////

cedar::closure::closure(int n) {
	if (n > CLOSURE_INTERNAL_SIZE) {
		printf("MUST USE EXTERNAL CLOSURE\n");
		use_vec = true;
		vars = std::vector<ref>(n);
	}
}

cedar::closure::~closure(void) {
}

ref & cedar::closure::at(int i) {
	return use_vec ? vars[i] : var_a[i];
}

/////////////////////////////////////////////////////

cedar::lambda::lambda() {
	code = std::make_shared<cedar::vm::bytecode>();
}


cedar::lambda::lambda(std::shared_ptr<vm::bytecode> bc) {
	type = bytecode_type;
	code = bc;
}

cedar::lambda::lambda(bound_function func) {
	type = function_binding_type;
	function_binding = func;
}

cedar::lambda::~lambda() {
}

cedar::runes lambda::to_string(bool human) {
	char addr_buf[30];
	if (type == bytecode_type) {
		std::sprintf(addr_buf, "%p", (void*)code.get());
	} else if (type == function_binding_type) {
		std::sprintf(addr_buf, "binding %p", (void*)function_binding);
	}

	cedar::runes str;
	str += "<lambda ";
	str += addr_buf;
	str += ">";
	return str;
}


ref lambda::to_number() {
	throw cedar::make_exception("Attempt to cast lambda to number failed");
}

u64 lambda::hash(void) {
	return reinterpret_cast<u64>(type == bytecode_type ? (void*)code.get() : (void*)function_binding);
}
