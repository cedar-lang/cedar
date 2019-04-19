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

#include <cedar/ast.h>
#include <cedar/object/dict.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/object/vector.h>
#include <cedar/objtype.h>
#include <cedar/ref.h>
#include <cedar/vm/machine.h>


using namespace cedar;
using namespace cedar::ast;




void ast::scope::finalize(void) {
  stack_size = 0;
  closure_size = 0;

  // finalize walks *down* the scope tree and decides on which variables go in
  // closures, and which dont. It also sets up closure indexes, instructions on
  // how to allocate closures, etc...
  //
  if (parent == nullptr) {
    closure_index = 0;
  } else {
    // any children of a function that has a closure must allocate a closure to
    // maintain access to variables. Imagine a function that returns a function
    // of no args, where that function returns another function that expects a
    // closed value form the root function. The 3rd function would have no way
    // to access the root function becasue the middle function did not have
    // access to the root's closure through the middle function
    allocate_closure = parent->allocate_closure;
    closure_index = parent->closure_index + parent->vars.size();
  }




  closure_size = 0;
  for (auto v : vars) {
    if (v->escapes) {
      allocate_closure = true;
      v->closure_index = closure_index + closure_size;
      closure_size += 1;
    } else {
      v->stack_index = stack_size;
      stack_size+=1;
    }
  };

  /*

  printf("+");
  for (int i = 0; i < closure_index-1; i++) printf(" ");
  printf("%2d %2d: %p\n", closure_size, closure_index, this);
  for (auto v : vars) {
    for (int i = 0; i < closure_index+1; i++) printf(" ");
    printf("%2d: %s\n", v->closure_index, v->str().c_str());
  };
  */

  for (auto c : children) {
    c->finalize();
  }
}




// helper function for the list compiler for checking if
// a list is a call to some special form function
static bool is_call(ref &obj, cedar::runes func_name) {
  cedar::list *list = ref_cast<cedar::list>(obj);
  if (list)
    if (auto func_name_given = ref_cast<cedar::symbol>(list->first());
        func_name_given) {
      return func_name_given->get_content() == func_name;
    }
  return false;
}

// the ast was an afterthought. so this function is a big ugly mess.
// This function turns any object into an AST node. It will macroexpand as
// needed
node *ast::parse(ref obj, module *m, scope *sc) {
  if (sc == nullptr) {
    sc = new scope(nullptr, m);
  }
  type *ot = obj.get_type();


  if (ot == list_type) {
    // the biggest possible ast node is a call.
    // first check if the call is a macro and parse that instead...
    {
      ref func_name = obj.first();
      symbol *s = ref_cast<symbol>(func_name);
      if (s != nullptr) {
        auto sid = s->id;
        if (vm::is_macro(sid)) {
          ref expanded = vm::macroexpand_1(obj, m);
          if (expanded != obj) {
            return parse(expanded, m, sc);
          }
        }
      }
      // wasn't a macro
    }


    {
      // if is the only form of branching in the language
      if (is_call(obj, "if")) {
        ref cond = obj.rest().first();
        ref tr = obj.rest().rest().first();
        ref fl = obj.rest().rest().rest().first();  // could be nil, (rest nil) === nil
        if_node *n = new if_node(sc);
        n->cond = parse(cond, m, sc);
        n->true_body = parse(tr, m, sc);
        n->false_body = parse(fl, m, sc);
        return n;
      }
      // wasn't an if
    }


    bool is_defmac = is_call(obj, "defmacro*");
    bool is_defdef = is_call(obj, "def*");
    bool is_defprv = is_call(obj, "def-private*");

    // general case def
    if (is_defmac || is_defdef || is_defprv) {
      def_node *n = new def_node(sc);
      n->macro = is_defmac;
      n->priv = is_defprv;
      symbol *s = ref_cast<symbol>(obj.rest().first());
      if (s == nullptr) {
        throw make_exception(
            "invalid syntax in def*. Must have symbol as second "
            "argument: ",
            obj);
      }

      n->dst = parse(obj.rest().first(), m, sc);
      n->val = parse(obj.rest().rest().first(), m, sc);
      return n;
    }



    // compile constant quotes
    if (is_call(obj, "quote")) {
      ref v = obj.rest().first();
      auto *c = new const_node(sc);
      c->val = v;
      return c;
    }


    if (is_call(obj, "return")) {
      ref v = obj.rest().first();
      auto *c = new return_node(sc);
      c->val = parse(v, m, sc);
      return c;
    }



    if (is_call(obj, "fn")) {
      // spawn a new scope for this function
      scope *ns = new scope(sc, m);
      static auto amp_sym = newsymbol("&");
      auto *fn = new function_node(ns);
      auto expr = obj;
      auto args = expr.rest().first();
      ref name = nullptr;
      if (!(args.isa(list_type) || args.isa(vector_type)) && !args.is_nil()) {
        if (args.isa(symbol_type)) {
          name = args;
        } else {
          throw cedar::make_exception("unknown name option on lambda literal: ",
                                      args);
        }
        expr = expr.rest();
        args = expr.rest().first();
      }
      fn->name = name.to_string(true);
      while (true) {
        if (args.is_nil()) break;
        auto arg = args.first();

        if (arg == amp_sym) {
          fn->vararg = true;
          args = args.rest();
          if (args.is_nil())
            throw cedar::make_exception("invalid vararg syntax in function: ",
                                        expr);
          if (!args.rest().is_nil())
            throw cedar::make_exception(
                "invalid vararg syntax in function - only one vararg argument "
                "name "
                "allowed: ",
                expr);
          continue;
        }
        if (arg.is_nil()) break;
        if (auto *sym = ref_cast<cedar::symbol>(arg); sym != nullptr) {
          auto v = ns->add_var(sym->id);
          fn->args.push_back(v);
          if (fn->vararg && !args.rest().is_nil()) {
            throw cedar::make_exception("function variable arguments invalid: ",
                                        expr);
          }

        } else {
          if (arg.is_nil()) {
            throw cedar::make_exception("lambda arguments must be symbols: ",
                                        expr);
          }
        }
        args = args.rest();
      }
      auto body = expr.rest().rest().first();
      fn->body = parse(body, m, ns);
      return fn;
    }

    if (is_call(obj, "do")) {
      auto n = new do_node(sc);
      obj = obj.rest();
      while (true) {
        n->body.push_back(parse(obj.first(), m, sc));
        if (obj.rest().is_nil()) break;
        obj = obj.rest();
      }
      return n;
    }

    if (is_call(obj, "let*")) {
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

      return parse(call, m, sc);
    }  // end of let construction

    if (is_call(obj, "scope*")) {
      auto ns = new scope(sc, m);

      auto n = new scope_node(ns);

      obj = obj.rest();

      auto args = obj.first();

      while (!args.is_nil()) {
        symbol *s = ref_cast<symbol>(args.first());
        if (s == nullptr) {
          throw make_exception();
        }

        auto v = ns->add_var(s->id);
        v->argument = false;

        n->names.push_back(v);
        args = args.rest();
      }

      obj = obj.rest();

      n->body = parse(obj.first(), m, ns);
      return n;
    }



    // if the parser got here, it's a normal funciton call
    std::vector<node *> arguments;

    ref args = obj.rest();
    while (!args.is_nil()) {
      arguments.push_back(parse(args.first(), m, sc));
      args = args.rest();
    }


    if (is_call(obj, "+")) {
      auto n = new math_op_node(sc);
      n->arguments = arguments;
      n->op = '+';
      return n;
    }


    if (is_call(obj, "-")) {
      auto n = new math_op_node(sc);
      n->arguments = arguments;
      n->op = '-';
      return n;
    }

    if (is_call(obj, "*")) {
      auto n = new math_op_node(sc);
      n->arguments = arguments;
      n->op = '*';
      return n;
    }

    if (is_call(obj, "/")) {
      auto n = new math_op_node(sc);
      n->arguments = arguments;
      n->op = '/';
      return n;
    }
    auto c = new call_node(sc);

    ref func = obj.first();
    c->func = parse(func, m, sc);
    c->arguments = arguments;

    return c;
  }


  if (ot == nil_type) {
    return new nil_node(sc);
  }

  if (ot == number_type) {
    auto n = new number_node(sc);
    if (obj.is_int()) {
      auto d = obj.to_int();
      n->is_float = false;
      n->i = d;
    } else {
      double d = obj.to_float();
      n->is_float = true;
      n->d = d;
    }
    return n;
  }



  if (ot == symbol_type) {
    symbol *s = ref_cast<symbol>(obj);
    auto n = new symbol_node(sc);
    auto v = sc->find(s->id);
    // if the scope is not the same, then the variable escapes
    if (v->sc != sc) {
      v->escapes = true;
    }
    v->used = true;
    n->binding = v;
    return n;
  }


  if (ot == vector_type) {
    vector *v = ref_cast<vector>(obj);

    auto len = v->size();

    auto n = new vector_node(sc);

    for (int i = 0; i < len; i++) {
      n->elems.push_back(parse(v->at(i), m, sc));
    }
    return n;
  }


  if (ot == keyword_type) {
    auto n = new keyword_node(sc);
    n->val = obj.to_string(true);
    return n;
  }


  if (ot == string_type) {
    auto n = new string_node(sc);
    n->val = obj.to_string(true);
    return n;
  }


  if (ot == dict_type) {
    auto n = new dict_node(sc);

    dict *d = ref_cast<dict>(obj);
    auto kv = d->keys();

    while (!kv.is_nil()) {
      ref pair = kv.first();
      ref key = pair.first();
      ref val = pair.rest().first();
      n->elems.insert(std::pair(parse(key, m, sc), parse(val, m, sc)));
      kv = kv.rest();
    }
    return n;
  }



  std::cout << "UNHANDLED NODE\n" << std::endl;

  exit(0);

  return new node(sc);
}
