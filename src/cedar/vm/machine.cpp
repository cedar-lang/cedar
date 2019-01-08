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

#include <cedar/object/lambda.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <cedar/ref.h>
#include <cedar/object.h>
#include <cedar/object/list.h>

using namespace cedar;

vm::machine::machine(void) : m_compiler(this) {
	stacksize = 512;
	stack = new ref[stacksize];
}


vm::machine::~machine() {
	delete[] stack;
}

ref vm::machine::eval(ref obj) {

	ref compiled_lambda = m_compiler.compile(obj);

	auto *raw_program = ref_cast<cedar::lambda>(compiled_lambda);

	if (raw_program == nullptr) {
		return compiled_lambda;
	}



#define VM_TRACE


	// declare and initialize the frame and stack pointers to 0
	uint64_t fp, sp;
	fp = sp = 0;

#define PUSH(val) stack[sp++] = val
#define PUSH_PTR(ptr) stack[sp++].store_ptr((void*)(ptr))
#define POP() stack[--sp]
#define POP_PTR() stack[--sp].reinterpret<void*>()
	// ip is the "instruction pointer" and dereferencing
	// it will select the opcode to run
	uint8_t *ip = nullptr;

#define CODE_READ(type) (*(type*)(void*)ip)
#define CODE_SKIP(type) (ip += sizeof(type))

#define LABEL(op) DO_##op
#define TARGET(op) DO_##op:



	void *threaded_labels[255];
	for (int i = 0; i < 255; i++)
		threaded_labels[i] = &&LABEL(OP_NOP);

#define SET_LABEL(op) threaded_labels[op] = &&DO_##op;

	SET_LABEL(OP_NIL);
	SET_LABEL(OP_CONST);
	SET_LABEL(OP_FLOAT);
	SET_LABEL(OP_INT);
	SET_LABEL(OP_LOAD_LOCAL);
	SET_LABEL(OP_SET_LOCAL);

	SET_LABEL(OP_LOAD_GLOBAL);
	SET_LABEL(OP_SET_GLOBAL);

	SET_LABEL(OP_CONS);
	SET_LABEL(OP_CALL);
	SET_LABEL(OP_MAKE_FUNC);
	SET_LABEL(OP_ARG_POP);
	SET_LABEL(OP_RETURN);


	ref progref = cedar::new_obj<cedar::lambda>(raw_program->code);




	// program is a pointer to the currently evaluating lambda expression.
	// In the background this is still managed by a managed refcount, but this
	// pointer is strictly a performance improvement in order to cut down on
	// dynamic_casts in a reference
	auto * program = ref_cast<cedar::lambda>(progref);

	// staring out, we need to copy all the useful information from the raw lambda
	// so we don't destroy the raw lambda's information such as closures or other
	// such constructs
	// copy the closure_size
	program->closure_size = raw_program->closure_size;
	// if the program has no closure usage, there should be no allocation. This is strictly
	// an optimization to speed up (only slightly) the initial "warm-up" of the VM
	if (program->closure_size != 0) {
		auto * c = new ref[program->closure_size];
		program->closure = std::shared_ptr<ref[]>(c);
	}
	// obtain a reference to the code
	program->code = raw_program->code;
	// and set the instruction pointer to the begining of this code.
	ip = program->code->code;

	// the calling convention requires that the frame pointer point at the arguments
	// of the current function being evaluated, so for top level expressions a nil must
	// be pushed to the stack for the calling convention to be obeyed correctly. Pushing
	// a `nullptr` is equivelent to pushing nil
	PUSH(nullptr);
	// the neTxt thing that must be pushed to the stack as per the calling convention is
	// the current code being evaluated. This will also be kept in a local variable in
	// the evaluation loop for quick, uninterupted access without having to go through a
	// slow dynamic_cast() for each call
	PUSH(program);

	// after pushing the lambda to the stack, we need to push the previous frame pointer,
	// which in this case is zero again
	PUSH_PTR(fp);
	// after the lambda that is being run, pushing the
	// the final thing to push is the previous instruction pointer, and since this is a
	// top level evaluation, pushing nullptr is the only value that really makes sense...
	PUSH(nullptr);


	uint64_t call_count = 0;

	// the currently evaluating opcode
	uint8_t op;
loop:

	// TODO: every n instructions, clean up the stack
	//       by setting all references above the sp to null
	//       also tweak this value intelligently

	// read the opcode from the instruction pointer
	op = *ip;

	// check if stack size must be reallocated
	if (sp >= stacksize) {
		auto new_size = stacksize + 512;
		auto *new_stack = new ref[new_size];
		for (uint64_t i = 0; i < sp-1; i++) {
			new_stack[i] = stack[i];
		}
		delete[] stack;
		stack = new_stack;
		stacksize = new_size;
	}

#ifdef VM_TRACE
	printf("%p: %02x %lu\n", ip, op, sp);
#endif
	ip++;
	goto *threaded_labels[op];

	// OP_NOP is currently a catchall. If this instruction is
	// encountered, an error is printed and the program just exits...
	// TODO: make it not do this and actually just ignore it.
	TARGET(OP_NOP) {
		fprintf(stderr, "UNHANDLED INSTRUCTION: %02x\n", op);
		exit(-1);
		goto loop;
	}


	TARGET(OP_NIL) {
		PUSH(nullptr);
		goto loop;
	}

	TARGET(OP_CONST) {
		const auto ind = CODE_READ(uint64_t);
		PUSH(program->code->constants[ind]);
		CODE_SKIP(uint64_t);
		goto loop;
	}

	TARGET(OP_FLOAT) {
		const auto flt = CODE_READ(double);
		PUSH(flt);
		CODE_SKIP(double);
		goto loop;
	}

	TARGET(OP_INT) {
		const auto integer = CODE_READ(int64_t);
		PUSH(integer);
		CODE_SKIP(int64_t);
		goto loop;
	}

	TARGET(OP_LOAD_LOCAL) {
		auto ind = CODE_READ(uint64_t);
		auto val = program->closure[ind];
		PUSH(val);
		CODE_SKIP(uint64_t);
		goto loop;
	}


	TARGET(OP_SET_LOCAL) {
		auto ind = CODE_READ(uint64_t);
		program->closure[ind] = POP();
		CODE_SKIP(uint64_t);
		goto loop;
	}

	TARGET(OP_LOAD_GLOBAL) {
		auto ind = CODE_READ(uint64_t);
		ref symbol = program->code->constants[ind];

		try {
			auto binding = global_bindings.at(symbol.symbol_hash());
			PUSH(std::get<ref>(binding));
		} catch (std::exception &) {
			throw cedar::make_exception("Symbol '", symbol, "' not bound");
		}

		CODE_SKIP(uint64_t);
		goto loop;
	}


	TARGET(OP_SET_GLOBAL) {
		auto ind = CODE_READ(uint64_t);
		ref symbol = program->code->constants[ind];

		global_bindings[symbol.symbol_hash()] = {symbol.to_string(), POP()};
		CODE_SKIP(uint64_t);
		goto loop;
	}

	TARGET(OP_CONS) {
		ref list = new_obj<cedar::list>();
		list.set_first(POP());
		list.set_rest(POP());
		PUSH(list);
		goto loop;
	}

	TARGET(OP_CALL) {

		printf("cc: %lu\n", call_count++);
		PUSH_PTR(ip);
		PUSH_PTR(fp);
		fp = sp - 4;
		program = stack[fp+1].reinterpret<cedar::lambda*>();
		ip = program->code->code;
		goto loop;
	}

	TARGET(OP_MAKE_FUNC) {

		auto ind = CODE_READ(uint64_t);
		ref function_template = program->code->constants[ind];
		auto *template_ptr = ref_cast<cedar::lambda>(function_template);

		ref function = new_obj<cedar::lambda>();
		auto *fptr = ref_cast<cedar::lambda>(function);

		// inherit closures from parent
		fptr->closure = program->closure;
		fptr->closure_size = program->closure_size;
		fptr->code = template_ptr->code;
		PUSH(fptr);
		CODE_SKIP(uint64_t);
		goto loop;
	}

	TARGET(OP_ARG_POP) {
		int ind = CODE_READ(uint64_t);
		ref arg = stack[fp].get_first();
		program->closure[ind] = arg;
		stack[fp] = stack[fp].get_rest();
		CODE_SKIP(uint64_t);
		goto loop;
	}

	TARGET(OP_RETURN) {

		std::cout << POP() << std::endl;
		return POP();
		goto loop;
	}


	goto loop;


	return nullptr;
}
