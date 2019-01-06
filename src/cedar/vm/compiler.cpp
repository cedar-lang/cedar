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

#include <cedar/vm/compiler.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <cedar/passes.h>
#include <cedar/ref.h>

#include <cedar/object/list.h>
#include <cedar/object/lambda.h>
#include <cedar/object/symbol.h>
#include <cedar/object/number.h>

using namespace cedar;


vm::compiler::compiler(cedar::vm::machine *vm) {
	m_vm = vm;
}
vm::compiler::~compiler() {}



ref vm::compiler::compile(ref obj) {

	// run the value through some optimization and
	// modification to the AST of the object before
	// compiling to bytecode
	ref compiled = passcontroller(obj, this)
			.pipe(vm::bytecode_pass, "bytecode emission") // compile the object to a code lambda
			.get();

	return compiled;
}

// helper function for the list compiler for checking if
// a list is a call to some special form function
static bool list_is_call_to(cedar::runes func_name, ref & obj) {
	cedar::list *list = ref_cast<cedar::list>(obj);
	if (list)
		if (auto func_name_given = ref_cast<cedar::symbol>(list->get_first()); func_name_given) {
			return func_name_given->get_content() == func_name;
		}
	return false;
}



///////////////////////////////////////////////////////
// the entry point for the bytecode pass
//
// This function requires that the object be wrapper
// in a lambda, as it will attempt to compile it as a
// lambda in the vm::compiler instance. Not passing the
// expected expression will result in undefined behavior

ref cedar::vm::bytecode_pass(ref obj, vm::compiler *c) {

	ref lambda = cedar::new_obj<cedar::lambda>();

	c->compile_object(obj, ref_cast<cedar::lambda>(lambda)->code);
	return lambda;
}




//////////////////////////////////////////////////////


void vm::compiler::compile_object(ref obj, vm::bytecode & code) {

	if (obj.is<cedar::list>()) {
		return compile_list(obj, code);
	}

	if (obj.is<cedar::number>()) {
		return compile_number(obj.to_float(), code);
	}

	std::cout << obj << ": UNKNOWN TYPE IN COMPILER\n";
}

void vm::compiler::compile_list(ref obj, vm::bytecode &) {
	if (list_is_call_to("set", obj)) {
		printf("is set call\n");
	}
}


void vm::compiler::compile_number(double number, vm::bytecode & code) {
	code.write((uint8_t)OP_FLOAT);
	code.write(number);
}

