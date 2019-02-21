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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cedar/object/keyword.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/object/nil.h>
#include <cedar/object/number.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <cedar/object/vector.h>
#include <cedar/passes.h>
#include <cedar/ref.h>
#include <cedar/vm/compiler.h>
#include <cedar/vm/instruction.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <memory>

using namespace cedar;

////////////////////////////////////////////////////////////
// scope::
vm::scope::scope(std::shared_ptr<scope> parent) { m_parent = parent; }

int vm::scope::find(uint64_t symbol) {
  if (m_bindings.count(symbol) == 1) {
    return m_bindings.at(symbol);
  }
  // if there is no parent, return -1
  if (m_parent == nullptr) return -1;
  // defer to the parent recursively
  return m_parent->find(symbol);
}

int vm::scope::find(ref &symbol) { return find(symbol.symbol_hash()); }

void vm::scope::set(uint64_t symbol, int ind) { m_bindings[symbol] = ind; }

void vm::scope::set(ref &symbol, int ind) {
  m_bindings[symbol.symbol_hash()] = ind;
}

vm::compiler::compiler(cedar::vm::machine *vm) { m_vm = vm; }
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
static bool list_is_call_to(cedar::runes func_name, ref &obj) {
  cedar::list *list = ref_cast<cedar::list>(obj);
  if (list)
    if (auto func_name_given = ref_cast<cedar::symbol>(list->first());
        func_name_given) {
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

  code->write_op(OP_RETURN);
  code->write_op(OP_EXIT);
  // finalize the code (sum up stack effect)
  code->finalize();
  // build a lambda around the code
  auto lambda = cedar::new_obj<cedar::lambda>(code);

  ref_cast<cedar::lambda>(lambda)->m_closure = nullptr;
  return lambda;
}

//////////////////////////////////////////////////////

void vm::compiler::compile_object(ref obj, vm::bytecode &code, scope_ptr sc,
                                  compiler_ctx *ctx) {
  if (obj.isa(list_type)) {
    return compile_list(obj, code, sc, ctx);
  }

  if (obj.is<cedar::vector>()) {
    return compile_vector(obj, code, sc, ctx);
  }

  if (obj.is_number()) {
    return compile_number(obj, code, sc, ctx);
  }

  if (obj.is<cedar::symbol>()) {
    return compile_symbol(obj, code, sc, ctx);
  }
  if (obj.is<cedar::nil>()) {
    code.write_op(OP_NIL);
    return;
  }

  // if we dont know what it is, just assume its a constant and move on...
  return compile_constant(obj, code, sc, ctx);
}

void cedar::vm::compiler::compile_call_arguments(ref args, bytecode &code,
                                                 scope_ptr sc,
                                                 compiler_ctx *ctx) {}

void vm::compiler::compile_progn(ref obj, vm::bytecode &code, scope_ptr sc,
                                 compiler_ctx *ctx) {
  while (true) {
    compile_object(obj.first(), code, sc, ctx);
    if (obj.rest().is_nil()) break;
    code.write_op(OP_SKIP);
    obj = obj.rest();
  }
}

void vm::compiler::compile_list(ref obj, vm::bytecode &code, scope_ptr sc,
                                compiler_ctx *ctx) {
  if (list_is_call_to("defmacro*", obj)) {
    ref name = obj.rest().first();
    ref func = obj.rest().rest().first();

    compile_object(func, code, sc, ctx);

    symbol *s = ref_cast<symbol>(name);
    if (s == nullptr) {
      throw cedar::make_exception("invalid defmacro. name must be a symbol");
    }

    code.write_op(OP_DEF_MACRO, s->id);

    return;
  }

  if (list_is_call_to("if", obj)) {
    obj = obj.rest();
    ref cond = obj.first();
    ref tru = obj.rest().first();
    ref fls = obj.rest().rest().first();

    compile_object(cond, code, sc, ctx);

    code.write_op(OP_JUMP_IF_FALSE);
    i64 false_join_loc = code.write((u64)0);

    // compile the true expression
    compile_object(tru, code, sc, ctx);
    // write a jump instruction to jump to after the true expr
    code.write_op(OP_JUMP);
    i64 true_join_loc = code.write((u64)0);

    // write to the false instruction's argument where to jump to
    code.write_to(false_join_loc, code.get_size());
    // compile the false expression
    compile_object(fls, code, sc, ctx);
    // write to the join location jump
    code.write_to(true_join_loc, code.get_size());

    return;
  }

  // def* is a special form that attempts to change a local closure binding
  // and if it can't, will set in the global scope
  if (list_is_call_to("def*", obj)) {
    auto name_obj = obj.rest().first();
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
      symbol *sym = name_obj.as<symbol>();
      i64 global_ind = sym->id;

      /*
      if (m_vm->global_symbol_lookup_table.find(hash) ==
          m_vm->global_symbol_lookup_table.end()) {
        // make space in global table
        global_ind = m_vm->global_table.size();

        vm::var v;
        v.value = nullptr;
        m_vm->global_table.push_back(v);

        m_vm->global_symbol_lookup_table[hash] = global_ind;
      } else {
        global_ind = m_vm->global_symbol_lookup_table.at(hash);
      }
      */

      opcode = OP_SET_GLOBAL;
      index = global_ind;
    }

    auto val_obj = obj.rest().rest().first();
    // compile the value onto the stack
    compile_object(val_obj, code, sc, ctx);

    code.write_op(opcode, index);
    return;
  }

  if (list_is_call_to("ns", obj)) {
    std::cout << obj << std::endl;
    return;
  }


  if (list_is_call_to("sleep", obj)) {

    ref arg = obj.rest().first();

    compile_object(arg, code, sc, ctx);
    code.write((u8)OP_SLEEP);
    return;
  }

  if (list_is_call_to("return", obj)) {
    ref arg = obj.rest().first();
    compile_object(arg, code, sc, ctx);
    code.write((u8)OP_RETURN);
    return;
  }

  if (list_is_call_to("quote", obj)) {
    return compile_constant(obj.rest().first(), code, sc, ctx);
  }

  if (list_is_call_to("fn", obj) || list_is_call_to("gen", obj)) {
    return compile_lambda_expression(obj, code, sc, ctx);
  }

  if (list_is_call_to("progn", obj) || list_is_call_to("do", obj)) {
    compile_progn(obj.rest(), code, sc, ctx);
    return;
  }

  // dot special form
  if (list_is_call_to(".", obj)) {
    ref curr = obj.rest();

    // load in the first item in the dot
    compile_object(curr.first(), code, sc, ctx);

    curr = curr.rest();

    while (!curr.is_nil()) {
      ref a = curr.first();

      if (a.is<symbol>()) {
        // compile getattr
        auto *s = a.as<symbol>();
        int id = s->id;

        // a symbol may be followed by a := in order to set the value
        // so we need to check it and set instead of get
        ref next = curr.rest().first();
        if (keyword *kw = ref_cast<keyword>(next); kw != nullptr) {
          if (kw->get_content() == ":=") {
            ref val = curr.rest().rest().first();
            compile_object(val, code, sc, ctx);
            code.write_op(OP_SET_ATTR, id);
            break;
          } else {
            throw cedar::make_exception("Unknown keyword '", next,
                                        "' in dot syntax: ", obj);
          }
        }

        code.write_op(OP_GET_ATTR, id);

      } else if (a.isa(list_type)) {
        // compile calls
        ref method_ref = a.first();
        ref args = a.rest();

        // duplicate the top of the stack
        code.write_op(OP_DUP, 1);

        if (!method_ref.is<symbol>()) {
          throw cedar::make_exception(
              "invalid syntax in dot special form: ", obj, " method call '",
              method_ref, "' must be a symbol");
        }

        int id = method_ref.as<symbol>()->id;

        code.write_op(OP_GET_ATTR, id);
        code.write_op(OP_SWAP);

        int argc = 1;

        while (!args.is_nil()) {
          argc++;
          compile_object(args.first(), code, sc, ctx);
          args = args.rest();
        }

        code.write_op(OP_CALL, argc);
      } else {
        throw cedar::make_exception("invalid syntax in dot special form: ",
                                    obj);
      }
      curr = curr.rest();
    }
    return;
  }

  // let is a special form, not expanded by macros
  // this version of let is only used when bootstrapping
  // the runtime, and a LET macro is defined that does a
  // more powerful macroexpand
  if (list_is_call_to("let*", obj)) {

    std::vector<ref> arg_names;
    std::vector<ref> arg_vals;

    ref args = obj.rest().first();
    ref body = obj.rest().rest();

    ref bname = args.first().first();
    ref bval = args.first().rest().first();
    ref brest = args.rest();

    ref bdy = new list(new symbol("do"), body);

    if (!brest.is_nil()) {
      bdy = newlist(obj.first() /* let* */, brest, bdy);
    }

    bool has_val = !bname.is_nil();

    if (has_val) bname = new list(bname, nullptr);

    ref fn = newlist(new symbol("fn"), bname, bdy);

    ref call = has_val ? newlist(fn, bval) : newlist(fn);


    return compile_object(call, code, sc, ctx);
  } // end of let construction





  if (list_is_call_to("eval", obj)) {
    compile_object(obj.rest().first(), code, sc, ctx);
    code.write_op(OP_EVAL);
    return;
  }

  {
    ref func_name = obj.first();
    symbol *s = ref_cast<symbol>(func_name);
    if (s != nullptr) {
      int sid = s->id;

      if (vm::is_macro(sid)) {
        ref expanded = macroexpand_1(obj);
        if (expanded != obj) {
          return compile_object(expanded, code, sc, ctx);
        }
      }
    }
  }

  bool special_form = false;

  i64 argc = -1;
  ref args = obj;

  // special forms like defmacro that pass unevaluated objects
  if (list_is_call_to("defmacro", obj)) {
    special_form = true;
    compile_object(args.first(), code, sc, ctx);
    argc = 0;
    args = args.rest();
  }

  bool recur_call = list_is_call_to("recur", obj);

  if (recur_call) {
    argc = 0;
    args = args.rest();
  }

  while (!args.is_nil()) {
    argc++;
    if (!special_form) {
      compile_object(args.first(), code, sc, ctx);
    } else {
      compile_constant(args.first(), code, sc, ctx);
    }
    args = args.rest();
  }

  if (recur_call) {
    code.write_op(OP_RECUR, argc);
    return;
  }

  code.write_op(OP_CALL, argc);
}

void vm::compiler::compile_vector(ref vec_ref, vm::bytecode &code, scope_ptr sc,
                                  compiler_ctx *ctx) {
  // load the vector constructor
  static ref vec_sym = new symbol("Vector");

  std::vector<ref> elems;

  vector *vec = ref_cast<vector>(vec_ref);

  if (vec == nullptr) {
    throw cedar::make_exception("compile_vector requires a vector... given ",
                                vec_ref);
  }

  int size = vec->size();

  for (int i = 0; i < size; i++) {
    elems.push_back(vec->at(i));
  }

  ref expanded;

  if (elems.size() > 0) {
    expanded = new list(vec_sym, new list(elems));
  } else {
    expanded = new list(vec_sym, nullptr);
  }

  compile_object(expanded, code, sc, ctx);
}

void vm::compiler::compile_number(ref obj, vm::bytecode &code, scope_ptr,
                                  compiler_ctx *) {
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

void vm::compiler::compile_constant(ref obj, bytecode &code, scope_ptr,
                                    compiler_ctx *) {
  auto const_index = code.push_const(obj);
  code.write((uint8_t)OP_CONST);
  code.write((uint64_t)const_index);
}

// symbol compilation can follow 1 of two paths. If the scope contains the
// symbol in it's map, it will be a local closure index and will be a fast O(1)
// lookup. If it isn't found in the map, the symbol lookup will defer to the
// global lookup system and be slightly slower
void vm::compiler::compile_symbol(ref sym, bytecode &code, scope_ptr sc,
                                  compiler_ctx *ctx) {
  auto nsym = [&](cedar::runes s) { return new_obj<symbol>(s); };

  cedar::runes str = sym.to_string();

  bool is_dot = false;
  for (auto it : str) {
    if (it == '.') {
      is_dot = true;
      break;
    }
  }

  if (is_dot && not(str == ".")) {
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
      } else
        rest += it;
    }

    if (rest.size() == 0)
      throw cedar::make_exception("invalid dot notation: ", sym);

    ref d = nsym(obj);
    ref key = nsym(rest);
    ref get = nsym(".");

    ref expr = newlist(get, d, key);

    return compile_object(expr, code, sc, ctx);
  }

  // if the symbol is found in the enclosing closure/freevars, just push the
  // constant time 'lookup' instruction
  if (const int ind = sc->find(sym); ind != -1) {
    code.write((uint8_t)OP_LOAD_LOCAL);
    code.write((uint64_t)ind);
    return;
  }


  /*
  u64 hash = sym.hash();
  if (m_vm->global_symbol_lookup_table.find(hash) ==
      m_vm->global_symbol_lookup_table.end()) {
    throw cedar::make_exception("Symbol '", sym, "' not found");
  }
  */

  symbol *symb = sym.as<symbol>();
  // auto ind = m_vm->global_symbol_lookup_table.at(hash);

  // grab the symbol from the global scope
  code.write((uint8_t)OP_LOAD_GLOBAL);
  code.write((i64)symb->id);
}

// lambda compilation is complicated becuase of the closure system behind the
// scenes. Closures need to be known at compile time, so top level lambda
// expressions need to construct the closure on their call, not construction
void vm::compiler::compile_lambda_expression(ref expr, bytecode &code,
                                             scope_ptr sc, compiler_ctx *ctx) {
  static auto amp_sym = newsymbol("&");

  static auto fn_sym = newsymbol("fn");
  static auto gen_sym = newsymbol("gen");

  ref func_sig_symbol = expr.first();


  ctx->lambda_depth++;

  bool is_top_level_lambda = ctx->lambda_depth == 1;

  auto new_scope = std::make_shared<scope>(sc);
  auto new_code = std::make_shared<bytecode>();

  if (is_top_level_lambda) {
  }

  auto args = expr.rest().first();

  ref name = nullptr;
  if (!args.isa(list_type) && !args.is_nil()) {
    if (args.is<symbol>()) {
      name = args;
    } else {
      throw cedar::make_exception("unknown name option on lambda literal: ",
                                  args);
    }
    expr = expr.rest();
    args = expr.rest().first();
  }

  // std::cout << "(fn " << name << " " << args << " " << ")\n";
  i32 argc = 0;
  i32 arg_index = ctx->closure_size;
  bool vararg = false;

  while (true) {
    if (args.is_nil()) break;
    auto arg = args.first();

    if (arg == amp_sym) {
      vararg = true;
      args = args.rest();
      if (args.is_nil())
        throw cedar::make_exception("invalid vararg syntax in function: ",
                                    expr);
      if (!args.rest().is_nil())
        throw cedar::make_exception(
            "invalid vararg syntax in function - only one vararg argument name "
            "allowed: ",
            expr);
      continue;
    }

    if (auto *sym = ref_cast<cedar::symbol>(arg); sym != nullptr) {
      new_scope->set(arg, ctx->closure_size);
      ctx->closure_size++;
      argc++;
      if (vararg && !args.rest().is_nil()) {
        throw cedar::make_exception("function variable arguments invalid: ",
                                    expr);
      }

    } else {
      if (arg.is_nil()) {
        throw cedar::make_exception("lambda arguments must be symbols: ", expr);
      }
    }

    args = args.rest();
  }

  auto body = expr.rest().rest().first();

  compile_object(body, *new_code, new_scope, ctx);
  new_code->write((uint8_t)OP_RETURN);

  ctx->lambda_depth--;

  // new_code->write((u8)OP_EXIT);
  new_code->finalize();
  auto *new_lambda = new lambda(new_code);
  new_lambda->vararg = vararg;
  new_lambda->argc = argc;
  new_lambda->arg_index = arg_index;
  new_lambda->name = name;

  const int const_ind = code.push_const(new_lambda);
  code.write((uint8_t)OP_MAKE_FUNC);
  code.write((uint64_t)const_ind);
}

void vm::compiler::compile_quasiquote(ref obj, vm::bytecode &bc,
                                      vm::scope_ptr sc, vm::compiler_ctx *ctx) {
  // std::cout << obj << std::endl;
  if (obj.is_nil()) {
    bc.write((u8)OP_NIL);
    bc.write((u8)OP_CONS);
    return;
  }

  if (list_is_call_to("unquote", obj)) {
    compile_object(obj.rest().first(), bc, sc, ctx);
    bc.write((u8)OP_CONS);
    return;
  }
  if (list_is_call_to("unquote-splicing", obj)) {
    compile_object(obj.rest().first(), bc, sc, ctx);
    bc.write((u8)OP_APPEND);
    return;
  }

  if (obj.isa(list_type)) {
    ref first = obj.first();
    ref rest = obj.rest();

    if (rest.is_nil()) {
      bc.write((u8)OP_NIL);
    } else {
      compile_quasiquote(rest, bc, sc, ctx);
    }
    compile_quasiquote(first, bc, sc, ctx);
    return;
  }

  auto ind = bc.push_const(obj);
  bc.write((u8)OP_CONST);
  bc.write((u64)ind);
  bc.write((u8)OP_CONS);

  return;
}
