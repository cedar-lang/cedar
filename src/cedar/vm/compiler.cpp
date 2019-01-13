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
#include <cedar/object/nil.h>
#include <cedar/object/list.h>
#include <cedar/object/lambda.h>
#include <cedar/object/symbol.h>
#include <cedar/object/keyword.h>
#include <cedar/object/number.h>
#include <cedar/object/string.h>
#include <memory>

using namespace cedar;



////////////////////////////////////////////////////////////
// scope::
vm::scope::scope(std::shared_ptr<scope> parent) {
	m_parent = parent;
}

int vm::scope::find(uint64_t symbol) {
	if (m_bindings.count(symbol) == 1) {
		return m_bindings.at(symbol);
	}
	// if there is no parent, return -1
	if (m_parent == nullptr)
		return -1;
	// defer to the parent recursively
	return m_parent->find(symbol);
}

int vm::scope::find(ref & symbol) {
	return find(symbol.symbol_hash());
}

void vm::scope::set(uint64_t symbol, int ind) {
	m_bindings[symbol] = ind;
}

void vm::scope::set(ref & symbol, int ind) {
	m_bindings[symbol.symbol_hash()] = ind;
}


vm::compiler::compiler(cedar::vm::machine *vm) {
	m_vm = vm;
}
vm::compiler::~compiler() {}



ref vm::compiler::compile(ref obj, vm::machine *machine) {

	// run the value through some optimization and
	// modification to the AST of the object before
	// compiling to bytecode
	auto controller = passcontroller(obj, this);

	controller.pipe(vm::bytecode_pass, "bytecode emission");

	ref compiled = controller.get();

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


	// make a top level bytecode object
	auto code = std::make_shared<cedar::vm::bytecode>();
	// make the top level scope for this expression
	auto sc = std::make_shared<scope>(nullptr);

	compiler_ctx context;

	c->compile_object(obj, *code, sc, &context);

	code->write((uint8_t)OP_EXIT);
	// finalize the code (sum up stack effect)
	code->finalize();
	// build a lambda around the code
	auto lambda = cedar::new_obj<cedar::lambda>(code);

	ref_cast<cedar::lambda>(lambda)->closure_size = context.closure_size;
	return lambda;
}




//////////////////////////////////////////////////////


void vm::compiler::compile_object(ref obj, vm::bytecode & code, scope_ptr sc, compiler_ctx* ctx) {

	if (obj.is<cedar::list>()) {
		return compile_list(obj, code, sc, ctx);
	}

	if (obj.is_number()) {
		return compile_number(obj, code, sc, ctx);
	}

	if (obj.is<cedar::symbol>()) {
		return compile_symbol(obj, code, sc, ctx);
	}

	if (obj.is<cedar::string>() || obj.is<cedar::keyword>()) {
		return compile_constant(obj, code, sc, ctx);
	}

	if (obj.is<cedar::nil>()) {
		code.write((uint8_t)OP_NIL);
		return;
	}


	std::cerr << "Unhandled object (type: " << obj.type_name() << ") " << obj << std::endl;
}


void cedar::vm::compiler::compile_call_arguments(ref args, bytecode & code, scope_ptr sc, compiler_ctx *ctx) {


	if (args.is_nil()) {
	}
	if (auto rest = args.get_rest(); rest.is_nil()) {
		code.write((uint8_t)OP_NIL);
	} else {
		compile_call_arguments(rest, code, sc, ctx);
	}

	compile_object(args.get_first(), code, sc, ctx);
	code.write((uint8_t)OP_CONS);
}



void vm::compiler::compile_progn(ref obj, vm::bytecode & code, scope_ptr sc, compiler_ctx *ctx) {
	while (true) {
		compile_object(obj.get_first(), code, sc, ctx);
		if (obj.get_rest().is_nil()) break;
		code.write((uint8_t)OP_SKIP);
		obj = obj.get_rest();
	}

}

void vm::compiler::compile_list(ref obj, vm::bytecode & code, scope_ptr sc, compiler_ctx* ctx) {
	// def is a special form that attempts to change a local closure binding
	// and if it can't, will set in the global scope
	if (list_is_call_to("def", obj)) {

		auto name_obj = obj.get_rest().get_first();
		if (!name_obj.is<symbol>()) {
			throw cedar::make_exception("Invalid syntax in def special form: ", obj);
		}

		auto val_obj = obj.get_rest().get_rest().get_first();

		// compile the value onto the stack
		compile_object(val_obj, code, sc, ctx);

		if (const int ind = sc->find(name_obj); ind != -1) {
			// is a local assignment
			code.write((uint8_t)OP_SET_LOCAL);
			code.write((uint64_t)ind);
		} else {
			// is a global assignment
			const int const_ind = code.push_const(name_obj);
			code.write((uint8_t)OP_SET_GLOBAL);
			code.write((uint64_t)const_ind);
		}

		return;
	}

	if (list_is_call_to("quote", obj)) {
		compile_constant(obj.get_rest().get_first(), code, sc, ctx);
		return;
	}

	if (list_is_call_to("lambda", obj)) {
		compile_lambda_expression(obj, code, sc, ctx);
		return;
	}

	if (list_is_call_to("progn", obj)) {
		compile_progn(obj.get_rest(), code, sc, ctx);
		return;
	}


	// otherwise it must be a method call, so compile using the calling convention
	compile_call_arguments(obj.get_rest(), code, sc, ctx);
	compile_object(obj.get_first(), code, sc, ctx);
	code.write((uint8_t)OP_CALL);
}

void vm::compiler::compile_number(ref obj, vm::bytecode & code, scope_ptr, compiler_ctx*) {

	if (obj.is_flt()) {
		code.write((uint8_t)OP_FLOAT);
		code.write(obj.to_float());
		return;
	}

	if (obj.is_int()) {
		code.write((uint8_t)OP_INT);
		code.write(obj.to_int());
		return;
	}
}

void vm::compiler::compile_constant(ref obj, bytecode & code, scope_ptr, compiler_ctx*) {
	auto const_index = code.push_const(obj);
	code.write((uint8_t)OP_CONST);
	code.write((uint64_t)const_index);
}



// symbol compilation can follow 1 of two paths. If the scope contains the symbol
// in it's map, it will be a local closure index and will be a fast O(1) lookup.
// If it isn't found in the map, the symbol lookup will defer to the global lookup
// system and be slightly slower
void vm::compiler::compile_symbol(ref sym, bytecode & code, scope_ptr sc, compiler_ctx *ctx) {

	auto nsym = [&](cedar::runes s) {
		return new_obj<symbol>(s);
	};

	cedar::runes str = sym.to_string();


	bool is_dot = false;
	for (auto it : str) {
		if (it == '.') {
			is_dot = true;
			break;
		}
	}

	if (is_dot) {
		cedar::runes obj;
		cedar::runes rest;

		bool encountered_dot = false;
		for (auto it : str) {
			if (it == '.') {
				if (encountered_dot) {
					obj += ".";
					obj += rest;
					rest = "";
				}
				encountered_dot = true;
				continue;
			}
			if (!encountered_dot) {
				obj += it;
			} else rest += it;
		}

		if (rest.size() == 0) throw cedar::make_exception("invalid dot notation on dict: ", sym);

		ref d = nsym(obj);
		ref key = nsym(rest);
		ref get = nsym("get");


		ref expr = newlist(get, d, newlist(nsym("quote"), key));

		return compile_object(expr, code, sc, ctx);

	}

	// if the symbol is found in the enclosing closure/freevars, just push the
	// constant time 'lookup' instruction
	if (const int ind = sc->find(sym); ind != -1) {
		code.write((uint8_t)OP_LOAD_LOCAL);
		code.write((uint64_t)ind);
		return;
	}

	const int const_index = code.push_const(sym);
	// lookup the symbol globally
	code.write((uint8_t)OP_LOAD_GLOBAL);
	code.write((uint64_t)const_index);

}



// lambda compilation is complicated becuase of the closure system behind the scenes.
// Closures need to be known at compile time, so top level lambda expressions need to
// construct the closure on their call, not construction
void vm::compiler::compile_lambda_expression(ref expr, bytecode & code, scope_ptr sc, compiler_ctx *ctx) {

	ctx->lambda_depth++;

	bool is_top_level_lambda = ctx->lambda_depth == 1;

	auto new_scope = std::make_shared<scope>(sc);
	auto new_code = std::make_shared<bytecode>();


	if (is_top_level_lambda) {
		new_code->write((u8)OP_MAKE_CLOSURE);
	}

	auto args = expr.get_rest().get_first();

	while (true) {

		if (args.is_nil()) break;
		auto arg = args.get_first();

		if (auto *sym = ref_cast<cedar::symbol>(arg); sym != nullptr) {
			new_code->write((uint8_t)OP_ARG_POP);
			new_code->write((uint64_t)ctx->closure_size);
			new_scope->set(arg, ctx->closure_size);
			ctx->closure_size++;
		} else {
			if (arg.is_nil()) {
				throw cedar::make_exception("lambda arguments must be symbols: ", expr);
			}
		}

		args = args.get_rest();
	}

	auto body = expr.get_rest().get_rest().get_first();

	compile_object(body, *new_code, new_scope, ctx);
	new_code->write((uint8_t)OP_RETURN);

	ctx->lambda_depth--;

	new_code->finalize();
	ref new_lambda = new_obj<cedar::lambda>(new_code);
	const int const_ind = code.push_const(new_lambda);
	code.write((uint8_t)OP_MAKE_FUNC);
	code.write((uint64_t)const_ind);

}
