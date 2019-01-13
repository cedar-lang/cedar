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

#include <chrono>
#include <ratio>
#include <array>

#include <cedar/object/lambda.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <cedar/ref.h>
#include <cedar/object.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/types.h>

using namespace cedar;

vm::machine::machine(void) : m_compiler(this) {
}


vm::machine::~machine() {
	// delete[] stack;
}




void vm::machine::bind(ref & symbol, ref value) {
	global_bindings[symbol.symbol_hash()] = {symbol.to_string(), value};
}


void vm::machine::bind(runes name, bound_function f) {
	ref symbol = new_obj<cedar::symbol>(name);
	ref lambda = new_obj<cedar::lambda>(f);
	global_bindings[symbol.symbol_hash()] = {name, lambda};
}

ref vm::machine::find(ref & symbol) {
	try {
		return std::get<ref>(global_bindings.at(symbol.symbol_hash()));
	} catch(...) {
		return nullptr;
	}
}


static const char *instruction_name(uint8_t op) {
	switch (op) {
#define OP_NAME(name, code, op_type, effect) case code: return #name;
		CEDAR_FOREACH_OPCODE(OP_NAME);
#undef OP_NAME
	}
	return "unknown";
}




static void *threaded_labels[255];
bool created_thread_labels = false;


ref vm::machine::eval(ref obj) {

	ref compiled_lambda = m_compiler.compile(obj, this);

	auto *raw_program = ref_cast<cedar::lambda>(compiled_lambda);

	if (raw_program == nullptr) {
		return compiled_lambda;
	}

	u64 stacksize = 2048;
	ref *stack = new ref[stacksize];

	i32 stack_size_required = 0;


	u64 fp, sp;
	fp = sp = 0;

	// ip is the "instruction pointer" and dereferencing
	// it will select the opcode to run
	uint8_t *ip = nullptr;

	// how high the "call stack" is
	u64 callheight = 0;

	// the currently evaluating opcode
	uint8_t op = OP_NOP;

	cedar::lambda * program = nullptr;

// #define VM_TRACE
#define USE_PREDICT


#define PUSH(val) (stack[sp++] = (val))
#define PUSH_PTR(ptr) (stack[sp++].store_ptr((void*)(ptr)))
#define POP() (stack[--sp])
#define POP_PTR() (stack[--sp].reinterpret<void*>())

#define CODE_READ(type) (*(type*)(void*)ip)
#define CODE_SKIP(type) (ip += sizeof(type))

#define LABEL(op) DO_##op
#define TARGET(op) DO_##op:



auto check_stack_size_and_resize = [&] (void) {
	if (sp >= stacksize - 64) {
		auto new_size = stacksize + 512;
		auto *new_stack = new ref[new_size];
		for (i64 i = 0; i < (i64)sp; i++) {
			new_stack[i] = stack[i];
		}
		delete[] stack;
		stack = new_stack;
		stacksize = new_size;
	}
};


#define DEFAULT_PRELUDE check_stack_size_and_resize();

#ifdef VM_TRACE

std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();

auto trace_current_state = [&](void) {

	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<long long, std::nano> dt = now - last_time;
	double dtns = dt.count();

	printf("op: %02x %-20.15s ", op, instruction_name(op));
	printf("Δt: %5.5fms  ", dtns / 1000.0 / 1000.0);
	printf("sp: %6lu  ", sp);
	printf("ch: %6lu  ", callheight);
	printf("\n");

	last_time = now;
};

#define PRELUDE DEFAULT_PRELUDE; trace_current_state();
/*
#define PRELUDE \
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();  \
	std::chrono::duration<long long, std::nano> dt = now - last_time; \
	double dtns = dt.count();                                         \
	printf("sp: %3zu, fp: %3zu, ip %p, preds: %4ld, prev_dt: %8.5fms, op: %s\n", (size_t)sp, (size_t)fp, ip, correct_predictions, dtns / 1000.0 / 1000.0, instruction_name(op)); \
	last_time = now;
	*/
#else
#define PRELUDE DEFAULT_PRELUDE
#endif
#define DISPATCH goto loop;






// some opcodes come in very common pairs, making it very possible to do speculative exec
// on what the next opcode could be... This macro allows high speed assumptions to be made
// at the end of an opcode that is typically followed by another opcode
// The performance loss by this check is minimal on modern CPUs because of the l1 cache typically
// caching 8 bytes, so the best case scenario is a cache miss 1/8 opcodes that predict
// the next instruction
long correct_predictions = 0;
#ifdef USE_PREDICT
#define PREDICT(pred, label) \
	do {              \
		if (*ip == pred) { \
			ip++;              \
			op = *ip;          \
			correct_predictions++; \
			goto label; \
		} else correct_predictions--; } while (0);

#else
#define PREDICT(pred, label)
#endif

#define SET_LABEL(op) threaded_labels[op] = &&DO_##op;


	// if the global thread label vector isn't initialized, we need to do that first
	if (!created_thread_labels) {
		for (int i = 0; i < 255; i++) threaded_labels[i] = &&LABEL(OP_NOP);
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
		SET_LABEL(OP_EXIT);
		SET_LABEL(OP_SKIP);

		SET_LABEL(OP_MAKE_CLOSURE);
		created_thread_labels = true;
	}


	ref progref = cedar::new_obj<cedar::lambda>(raw_program->code);


	// program is a pointer to the currently evaluating lambda expression.
	// In the background this is still managed by a managed refcount, but this
	// pointer is strictly a performance improvement in order to cut down on
	// dynamic_casts in a reference
	program = ref_cast<cedar::lambda>(progref);

	// staring out, we need to copy all the useful information from the raw lambda
	// so we don't destroy the raw lambda's information such as closures or other
	// such constructs
	// copy the closure_size
	program->closure_size = raw_program->closure_size;
	// if the program has no closure usage, there should be no allocation. This is strictly
	// an optimization to speed up (only slightly) the initial "warm-up" of the VM
	if (program->closure_size != 0) {
		program->closure = std::make_shared<closure>(program->closure_size);
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


loop:

	// printf("preds: %ld\n", correct_predictions);
	// TODO: every n instructions, clean up the stack
	//       by setting all references above the sp to null
	//       also tweak this value intelligently

	// read the opcode from the instruction pointer
	op = *ip;

	// check if stack size must be reallocated
	/*
	if (sp >= stacksize-128) {
		auto new_size = stacksize + 512;
		auto *new_stack = new ref[new_size];
		for (uint64_t i = 0; i < sp-1; i++) {
			new_stack[i] = stack[i];
		}
		delete[] stack;
		stack = new_stack;
		stacksize = new_size;
	}
	*/

	// for (unsigned int i = sp; i < stacksize; i++) {
	// 	stack[i] = nullptr;
	// }


	ip++;
	goto *threaded_labels[op];

	// OP_NOP is currently a catchall. If this instruction is
	// encountered, an error is printed and the program just exits...
	// TODO: make it not do this and actually just ignore it.
	TARGET(OP_NOP) {
		PRELUDE;
		fprintf(stderr, "UNHANDLED INSTRUCTION: %02x\n", op);
		exit(-1);
		DISPATCH;
	}


	TARGET(OP_NIL) {
		PRELUDE;
		PUSH(nullptr);
		DISPATCH;
	}

	TARGET(OP_CONST) {
		PRELUDE;
		const auto ind = CODE_READ(uint64_t);
		PUSH(program->code->constants[ind]);
		CODE_SKIP(uint64_t);
		DISPATCH;
	}

	TARGET(OP_FLOAT) {
		PRELUDE;
		const auto flt = CODE_READ(double);
		PUSH(flt);
		CODE_SKIP(double);
		DISPATCH;
	}

	TARGET(OP_INT) {
		PRELUDE;
		const auto integer = CODE_READ(i64);
		PUSH(integer);
		CODE_SKIP(i64);
		DISPATCH;
	}

	TARGET(OP_LOAD_LOCAL) {
		PRELUDE;
		auto ind = CODE_READ(u64);
		auto val = program->closure->at(ind);
		PUSH(val);
		CODE_SKIP(u64);
		// load locals are typically packed into an argument
		// list, so it's reasonable to predict a possible
		// CONS operation immediately afterwards. If the prediction
		// was invalid, then it's not a problem, performace wise
		// thanks to the CPU cache
		PREDICT(OP_CONS, DO_OP_CONS);
		DISPATCH;
	}


	TARGET(OP_SET_LOCAL) {
		PRELUDE;
		auto ind = CODE_READ(u64);
		ref val = POP();
		program->closure->at(ind) = val;
		PUSH(val);
		CODE_SKIP(u64);
		DISPATCH;
	}

	TARGET(OP_LOAD_GLOBAL) {
		PRELUDE;
		auto ind = CODE_READ(u64);
		ref symbol = program->code->constants[ind];

		if (!symbol.is<cedar::symbol>()) {

			std::cout << ref{program} << std::endl;
			for (auto i : program->code->constants) {
				std::cout << i << std::endl;
			}
			printf("Not a symbol %lu\n", ind);
		}

		try {
			auto binding = global_bindings.at(symbol.symbol_hash());
			PUSH(std::get<ref>(binding));
		} catch (std::exception &) {
			throw cedar::make_exception("Symbol '", symbol, "' not bound");
		}

		CODE_SKIP(u64);
		DISPATCH;
	}


	TARGET(OP_SET_GLOBAL) {
		PRELUDE;
		auto ind = CODE_READ(u64);

		ref symbol = program->code->constants[ind];
		ref val = POP();

		bind(symbol, val);

		PUSH(val);
		CODE_SKIP(u64);
		DISPATCH;
	}

	TARGET(OP_CONS) {
		PRELUDE;
		auto list_obj = new cedar::list();

		list_obj->set_first(POP());
		list_obj->set_rest(POP());

		ref list = list_obj;
		PUSH(list);
		DISPATCH;
	}

	TARGET(OP_CALL) {
		PRELUDE;
		PUSH_PTR(ip);
		PUSH_PTR(fp);
		fp = sp - 4;
		program = stack[fp+1].reinterpret<cedar::lambda*>();
		callheight++;
		if (program->type == lambda::bytecode_type) {
			stack_size_required = program->code->stack_size;
			ip = program->code->code;
		} else if (program->type == lambda::function_binding_type) {
			ref val = program->function_binding(stack[fp], this);
			PUSH(val);
			goto LABEL(OP_RETURN);
		}
		DISPATCH;
	}

	TARGET(OP_MAKE_FUNC) {
		PRELUDE;
		auto ind = CODE_READ(u64);
		ref function_template = program->code->constants[ind];
		auto *template_ptr = ref_cast<cedar::lambda>(function_template);

		ref function = new_obj<cedar::lambda>();
		auto *fptr = ref_cast<cedar::lambda>(function);

		// inherit closures from parent
		fptr->closure = program->closure;
		fptr->closure_size = program->closure_size;
		fptr->code = template_ptr->code;
		PUSH(fptr);
		CODE_SKIP(u64);
		DISPATCH;
	}

	TARGET(OP_ARG_POP) {
		PRELUDE;
		i32 ind = CODE_READ(u64);
		ref arg = stack[fp].get_first();
		program->closure->at(ind) = arg;
		stack[fp] = stack[fp].get_rest();
		CODE_SKIP(u64);
		DISPATCH;
	}

	TARGET(OP_RETURN) {
		PRELUDE;
		ref val = POP();
		auto prevfp = fp;
		fp = (u64)POP_PTR();
		ip = (u8*)POP_PTR();
		sp = prevfp;
		program = ref_cast<cedar::lambda>(stack[fp+1]);
		stack_size_required = program->code->stack_size;
		callheight--;
		PUSH(val);
		DISPATCH;
	}

	TARGET(OP_EXIT) {
		PRELUDE;
		goto end;
		DISPATCH;
	}


	TARGET(OP_SKIP) {
		PRELUDE;
		ref a = POP();
		DISPATCH;
	}


	TARGET(OP_MAKE_CLOSURE) {
		PRELUDE;
		program->closure = std::make_shared<closure>(program->closure_size);
		DISPATCH;
	}


	goto loop;
end:


	delete[] stack;

	return POP();
}
