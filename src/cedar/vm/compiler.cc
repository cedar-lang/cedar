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
#include <cedar/passes.h>
#include <cedar/ref.hpp>

using namespace cedar;


vm::compiler::compiler(cedar::vm::machine *vm) {
	m_vm = vm;
}
vm::compiler::~compiler() {}

ref vm::compiler::compile(ref obj) {

	passcontroller controller(obj);

	// run the value through some optimization and
	// modification to the AST of the object before
	// compiling to bytecode
	ref final_value =
		controller
		// top level objects must be lambdas, as that's
		// all that the evaluator knows how to evaluate
		// by passing 0 arguments to it
		.pipe(wrap_top_level_with_lambdas)
		.get();

	return obj;
}
