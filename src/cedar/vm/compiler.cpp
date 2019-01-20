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
#include <cedar/vm/instruction.h>
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

  ref_cast<cedar::lambda>(lambda)->closure = nullptr;
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
	if (obj.is<cedar::nil>()) {
		code.write((uint8_t)OP_NIL);
		return;
	}

  // if we dont know what it is, just assume its a constant and move on...
	return compile_constant(obj, code, sc, ctx);

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

	if (list_is_call_to("if", obj)) {
		obj = obj.get_rest();
		ref cond = obj.get_first();
		ref tru = obj.get_rest().get_first();
		ref fls = obj.get_rest().get_rest().get_first();


		compile_object(cond, code, sc, ctx);

		code.write((u8)OP_JUMP_IF_FALSE);
		i64 false_join_loc = code.write((u64)0);

		// compile the true expression
		compile_object(tru, code, sc, ctx);
		// write a jump instruction to jump to after the true expr
		code.write((u8)OP_JUMP);
		i64 true_join_loc = code.write((u64)0);


		// write to the false instruction's argument where to jump to
		code.write_to(false_join_loc, code.get_size());
		// compile the false expression
		compile_object(fls, code, sc, ctx);
		// write to the join location jump
		code.write_to(true_join_loc, code.get_size());

		return;
	}
	// def is a special form that attempts to change a local closure binding
	// and if it can't, will set in the global scope
	if (list_is_call_to("def", obj)) {

		auto name_obj = obj.get_rest().get_first();
		if (!name_obj.is<symbol>()) {
			throw cedar::make_exception("Invalid syntax in def special form: ", obj);
		}

    u8 opcode = 0;
    i64 index = 0;


		if (const int ind = sc->find(name_obj); ind != -1) {
			// is a local assignment
      opcode = OP_SET_LOCAL;
      index = ind;
		} else {

      u64 hash = name_obj.hash();

      i64 global_ind = 0;

      if (m_vm->global_symbol_lookup_table.find(hash) == m_vm->global_symbol_lookup_table.end()) {
        // make space in global table
        global_ind = m_vm->global_table.size();
        m_vm->global_table.push_back(nullptr);

        m_vm->global_symbol_lookup_table[hash] = global_ind;
      } else {
        global_ind = m_vm->global_symbol_lookup_table.at(hash);
      }
      opcode = OP_SET_GLOBAL;
      index = global_ind;
		}

		auto val_obj = obj.get_rest().get_rest().get_first();
		// compile the value onto the stack
		compile_object(val_obj, code, sc, ctx);
    code.write(opcode);
    code.write(index);

		return;
	}

	if (list_is_call_to("quote", obj)) {
		compile_constant(obj.get_rest().get_first(), code, sc, ctx);
		return;
	}

	if (list_is_call_to("lambda", obj) || list_is_call_to("fn", obj)) {
		compile_lambda_expression(obj, code, sc, ctx);
		return;
	}

	if (list_is_call_to("progn", obj) || list_is_call_to("do", obj)) {
		compile_progn(obj.get_rest(), code, sc, ctx);
		return;
	}





  // let is a special form, not expanded by macros
  if (list_is_call_to("let", obj)) {

    std::vector<ref> arg_names;
    std::vector<ref> arg_vals;

    ref args = obj.get_rest().get_first();
    ref body = obj.get_rest().get_rest();


    if (!args.is_nil()) {
      if (!args.is<list>()) throw cedar::make_exception("let expression requires second argument to be list of bindings");

      while (true) {
        if (args.is_nil()) break;
        ref arg = args.get_first();
        arg_names.push_back(arg.get_first());
        arg_vals.push_back(arg.get_rest().get_first());
        args = args.get_rest();
      }
    }

    ref argobj = arg_names.size() == 0 ? nullptr : new_obj<list>(arg_names);

    auto func = newlist(new_obj<symbol>("fn"), argobj, new_obj<list>(new_obj<symbol>("progn"), body));

    std::vector<ref> call;
    call.push_back(func);
    for (auto it : arg_vals) {
      call.push_back(it);
    }

    ref expr = new_obj<list>(call);

    return compile_object(expr, code, sc, ctx);
  }


  if (list_is_call_to("eval", obj)) {
    compile_object(obj.get_rest().get_first(), code, sc, ctx);
    code.write((uint8_t)OP_EVAL);
    return;
  }


  bool special_form = false;


	i64 argc = -1;
	ref args = obj;


  // special forms like defmacro that pass unevaluated objects
  if (list_is_call_to("defmacro", obj)) {
    special_form = true;
    compile_object(args.get_first(), code, sc, ctx);
    argc = 0;
    args = args.get_rest();
  }



	bool recur_call = list_is_call_to("recur", obj);

	if (recur_call) {
		argc = 0;
		args = args.get_rest();
	}

	while (!args.is_nil()) {
		argc++;
    if (!special_form) {
      compile_object(args.get_first(), code, sc, ctx);
    } else {
      compile_constant(args.get_first(), code, sc, ctx);
    }
		args = args.get_rest();
	}


  if (recur_call) {
    code.write((uint8_t)OP_RECUR);
    code.write((i64)argc);
    return;
  }

  code.write((uint8_t)OP_CALL);
  code.write((i64)argc);

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

  u64 hash = sym.hash();

  if (m_vm->global_symbol_lookup_table.find(hash) == m_vm->global_symbol_lookup_table.end()) {
    throw cedar::make_exception("Symbol '", sym, "' not found");
  }

  auto ind = m_vm->global_symbol_lookup_table.at(hash);

  // grab the symbol from the global scope
	code.write((uint8_t)OP_LOAD_GLOBAL);
	code.write((i64)ind);

}



// lambda compilation is complicated becuase of the closure system behind the scenes.
// Closures need to be known at compile time, so top level lambda expressions need to
// construct the closure on their call, not construction
void vm::compiler::compile_lambda_expression(ref expr, bytecode & code, scope_ptr sc, compiler_ctx *ctx) {

  auto amp_sym = newsymbol("&");

	ctx->lambda_depth++;

	bool is_top_level_lambda = ctx->lambda_depth == 1;

	auto new_scope = std::make_shared<scope>(sc);
	auto new_code = std::make_shared<bytecode>();


	if (is_top_level_lambda) {
	}

	auto args = expr.get_rest().get_first();

	i32 argc = 0;
	i32 arg_index = ctx->closure_size;
  bool vararg = false;

	while (true) {

		if (args.is_nil()) break;
		auto arg = args.get_first();

    if (arg == amp_sym) {
      vararg = true;
      args = args.get_rest();
      continue;
    }

		if (auto *sym = ref_cast<cedar::symbol>(arg); sym != nullptr) {
			new_scope->set(arg, ctx->closure_size);
			ctx->closure_size++;
			argc++;
      if (vararg && !arg.get_rest().is_nil()) {
        throw cedar::make_exception("funciton variable arguments invalid: ", expr);
      }

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
	auto *new_lambda = new lambda(new_code);
	new_lambda->vararg = vararg;
	new_lambda->argc = argc;
	new_lambda->arg_index = arg_index;


	const int const_ind = code.push_const(new_lambda);
	code.write((uint8_t)OP_MAKE_FUNC);
	code.write((uint64_t)const_ind);

}
