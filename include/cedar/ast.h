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


#pragma once
#ifndef __AST__
#define __AST__

#include <cedar/jit.h>
#include <cedar/object/symbol.h>
#include <cedar/ref.h>
#include <map>
#include <vector>

namespace cedar {
  namespace ast {

    struct scope;


    // The old bytecode compiler used closures for every argument. This was more
    // than likely the slowest part of the language, because most funcitons did
    // not use or need closures. In the JIT, we can allocate arguments on the
    // stack perfectly fine without much headache. Knowing this, we should go
    // ahead and limit closure usage as much as we can.
    //
    // To do this, each abstract 'variable' in the AST has a few attributes on
    // it. When in the ast parser, every access of a variable is recorded, and
    // if a variable is accessed in a scope outside of its own (in a child
    // scope), then that variable is considered to have 'escaped'. This
    // partially resembles a very crude form of escape analysis. It allows the
    // JIT to know what variables need to be stored where. While this will make
    // compliation more complicated as there are now 2 logical variable types,
    // it will hopefully improve the performance of functions that have no
    // escaping variables like a basic wrapper function. For example:
    //
    // (defn first [x]
    //    (x.first))
    //
    // This function is a basic wrapper around the 'first' method on some 'x'
    // variable. There's a problem in the old interpreter where this simple call
    // would allocate a huge span of memory for A) the closure to store 'x' in,
    // B) the interpreter stack, and C) the call frame. In a JIT enviroment, we
    // can remove all of these requirements with this function. The 'x' variable
    // does not escape the function, and can therefore be loaded on the stack.
    // Because there is no escaping variables, there also does not need to be
    // any closure allocations.
    //
    // In the situation where a closure is used, only variables that escape are
    // allocated to the closure. Variables are stored in the order of the
    // arguments. This allows a simple walk to see the order of arguments. In
    // doing this, the compiler can add closed values to a closure and stack
    // values to the stack. It also knows how big any closure is ahead of time,
    // so all of this can be done statically in x86 machine code.
    //
    // In the case where a closure does occur, all functions after it must also
    // have a closure to support walking up the closure chain to find the
    // variable it needs. If a function has 0 arguments in a closure, it can
    // simply inherit the closure from the parent to save A) allocation, and
    // B) expensive closure walking.
    //
    // Variables will be further abstracted in the compiler with abstract
    // classes for where they are stored. Variables will have a `.access(cc)`
    // method that is overloaded based on the type of variable currently being
    // compiled. A stack allocated variable will access the function_callback
    // struct and a closure allocated variable will access the closure in which
    // it is defined.
    //
    // There are also 2 kinds of 'stack' variables. Arguments and Scoped
    // variables. Arguments can be accessed in the function_callback struct
    // using an `lea` on the argv field. This saves memory copies. Scoped
    // variables are actually on the stack and can be accessed fairly quickly.
    // All stack variables are arguments unless specified (scope* is the special
    // case)
    //
    // Scope* variables are a special case for 2 reasons. A) they are allocated
    // on the stack and are not arguments, and B) they can also be closed over.
    // When the JIT enters a scope, it will allocate stack space for vars that
    // do not escape, and will allocate a closure for the vars that do. It will
    // then modify the current function's closure by adding the new closure (if
    // it needs to) with the old closure as it's parent. Upon exiting the scope,
    // the previous closure is restored (if a closure was allocated at all)
    struct var {

      enum var_type {
        reference,
        integer,
        floating,
      };
      bool global = false;
      // if a var escapes, it must be in a closure somewhere.
      bool escapes = false;
      uint64_t id;
      bool vararg = false;
      bool used = false;
      bool argument = true;
      int closure_index = -1;
      int stack_index = -1;
      // if there is no scope, it's global
      scope *sc = nullptr;

      inline std::string str() {
        std::string s = "<";

        s += symbol::unintern(id);

        std::string flags = "";
        flags += escapes ? "e" : "";
        flags += global ? "g" : "";
        flags += vararg ? "v" : "";
        flags += !used ? "!" : "";

        if (flags != "") {
          s += " " + flags;
        }
        s += ">";
        return s;
      }
    };


    struct function_node;

    struct scope {
      scope *parent;
      module *mod;
      function_node *func;
      int closure_size;
      int closure_index;
      bool allocate_closure = false;
      std::vector<scope*> children;
      std::vector<var *> vars;
      void finalize(void);
      inline scope(scope *p = nullptr, module *m = nullptr) {
        parent = p;
        mod = m;
        if (parent != nullptr) {
          parent->children.push_back(this);
        }
      }

      inline var *add_var(uint64_t id) {
        var *v = new var();
        v->id = id;
        v->sc = this;
        vars.push_back(v);
        return v;
      }

      inline var *find(uint64_t id) {
        for (auto v : vars) {
          if (v->id == id) {
            return v;
          }
        }
        if (parent != nullptr) {
          return parent->find(id);
        }
        auto v = new var();
        v->id = id;
        v->sc = nullptr;
        v->global = true;
        return v;
      }
    };


    //====================================
    //====================================
    //====================================
    //====================================
    //====================================
    //====================================
    //====================================
    //====================================
    //====================================

    struct node {
      enum node_type {
        unknown,
        number,
        symbol,
        call,
        constant,
        string,
        keyword,
        function,
        dict,
        vector,
        do_,
        def,
        nil,
        dot,
        if_,
        return_,
        scope_,
        math_op,
      };

      node_type type = unknown;

      scope *sc;

      virtual std::string str() { return ":unknown-node"; }

      inline node(scope *s) { sc = s; }
      inline virtual ~node(){};
    };


    struct number_node : public node {
      bool is_float;
      double d;
      int64_t i;
      node_type type = number;
      using node::node;


      virtual std::string str() {
        std::string s = "";
        if (is_float) {
          s += std::to_string(d);
        } else {
          s += std::to_string(i);
        }
        return s;
      }

      inline virtual ~number_node(){/* STUB */};
    };

    struct symbol_node : public node {
      var *binding;
      node_type type = symbol;
      using node::node;
      virtual std::string str() { return binding->str(); }
      inline virtual ~symbol_node(){/* STUB */};
    };

    struct call_node : public node {
      node *func;
      std::vector<node *> arguments;
      node_type type = call;
      using node::node;

      virtual std::string str() {
        std::string s = "(";
        s += func->str();
        s += " ";
        for (auto a : arguments) {
          s += a->str();
          s += " ";
        }
        s += ") ";
        return s;
      }
      inline virtual ~call_node(){/* STUB */};
    };

    struct string_node : public node {
      cedar::runes val;
      std::vector<node *> arguments;
      node_type type = string;
      using node::node;
      virtual std::string str() { return val; }
      inline virtual ~string_node(){/* STUB */};
    };

    struct keyword_node : public node {
      cedar::runes val;
      std::vector<node *> arguments;
      node_type type = keyword;
      using node::node;
      virtual std::string str() { return val; }
      inline virtual ~keyword_node(){/* STUB */};
    };


    struct function_node : public node {
      std::string name;
      std::vector<var *> args;
      node *body;
      bool vararg = false;
      node_type type = function;
      using node::node;
      virtual std::string str() {
        std::string s = "(fn ";
        s += name;
        s += " (";
        for (auto a : args) {
          s += a->str();
          s += " ";
        }
        s += ") ";
        s += body->str();
        s += ") ";
        return s;
      }
      inline virtual ~function_node(){/* STUB */};
    };


    struct vector_node : public node {
      std::vector<node *> elems;
      node_type type = vector;
      using node::node;
      virtual std::string str() {
        std::string s;
        s += "[";
        for (auto a : elems) {
          s += a->str();
          s += ",";
        }
        s += "]";
        return s;
      }
      inline virtual ~vector_node(){/* STUB */};
    };

    struct dict_node : public node {
      std::map<node *, node *> elems;
      node_type type = call;
      using node::node;
      virtual std::string str() {
        std::string s;
        s += "{";
        for (auto a : elems) {
          s += a.first->str();
          s += " ";
          s += a.second->str();
          s += ",";
        }
        s += "}";
        return s;
      }
      inline virtual ~dict_node(){/* STUB */};
    };

    struct do_node : public node {
      std::vector<node *> body;
      node_type type = do_;
      using node::node;
      virtual std::string str() {
        std::string s;
        s += "(do ";
        for (auto a : body) {
          s += a->str();
          s += " ";
        }
        s += ")";
        return s;
      }
      inline virtual ~do_node(){/* STUB */};
    };

    struct def_node : public node {
      node *dst;
      bool macro = false;
      bool priv = false;
      node *val;
      using node::node;
      node_type type = def;
      virtual std::string str() {
        std::string s = "(def";

        if (macro) s += "macro";
        if (priv) s += "private";
        s += "* ";

        s += dst->str();
        s += " ";
        s += val->str();
        s += ")";
        return s;
      }
      inline virtual ~def_node(){/* STUB */};
    };



    // const node is used for unknown types, or the quote operator
    struct const_node : public node {
      ref val;
      node_type type = constant;
      using node::node;

      virtual std::string str() {
        std::string s = "<const ";
        s += val.to_string(false);
        s += ">";
        return s;
      }
      inline virtual ~const_node(){/* STUB */};
    };



    // (. obj <attrs...> [(call ...)])
    // the dot node allows syntax to deeply nest into an object by getting
    // attributes and optionally finally call a method
    // so (. "hello" (len)) or len('hello') in python, expands into
    // - obj = "hello"
    // - attrs = [intern('len')]
    // - has_call = true (call the last attr)
    // - call_args = [] // has an implicit self argument of the form `(len
    // self)` the implicit self is why the call portion exists here. It allows
    // the compiler to optimize the 'self' access and limit lookups.
    struct dot_node : public node {
      node *obj;
      std::vector<uint64_t> attrs;
      bool has_call;
      std::vector<node *> call_args;
      node_type type = dot;
      using node::node;
      virtual std::string str() {
        // TODO
        std::string s = "DOT";
        return s;
      }
      inline virtual ~dot_node(){/* STUB */};
    };


    struct if_node : public node {
      node *cond;
      node *true_body;
      node *false_body;
      using node::node;
      node_type type = if_;

      virtual std::string str() {
        std::string s = "(if ";
        s += true_body->str();
        s += " ";
        s += false_body->str();
        s += ")";
        return s;
      }

      inline virtual ~if_node(){/* STUB */};
    };


    struct return_node : public node {
      node *val;
      using node::node;
      node_type type = return_;

      virtual std::string str() {
        std::string s = "(return ";
        s += val->str();
        s += ")";
        return s;
      }

      inline virtual ~return_node(){/* STUB */};
    };


    struct nil_node : public node {
      using node::node;
      node_type type = nil;

      virtual std::string str() { return "nil"; }

      inline virtual ~nil_node(){/* STUB */};
    };



    struct scope_node : public node {
      using node::node;
      node *body;
      std::vector<var *> names;
      node_type type = scope_;

      virtual std::string str() {
        std::string s;
        s += "(scope* [";
        for (auto v : names) {
          s += v->str() + ", ";
        }
        s += "] " + body->str() + ")";


        return s;
      }

      inline virtual ~scope_node(){/* STUB */};
    };



    struct math_op_node : public node {
      char op = '+';
      std::vector<node *> arguments;
      using node::node;
      node_type type = math_op;

      virtual std::string str() {

        std::string s = "(<math op> ";
        s += op;
        s += " ";
        for (auto a : arguments) {
          s += a->str();
          s += " ";
        }
        s += ") ";

        return s;
      }

      inline virtual ~math_op_node(){/* STUB */};
    };




    node *parse(ref, module *, scope *sc = nullptr);
  }  // namespace ast
}  // namespace cedar

#endif
