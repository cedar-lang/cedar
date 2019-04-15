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
#include <cedar/ref.h>
#include <map>
#include <vector>

namespace cedar {
  namespace ast {

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
        dot,
        if_,
      };

      node_type type = unknown;
      virtual ~node();
    };


    struct number_node : public node {
      bool is_float;
      double d;
      int64_t i;
      node_type type = number;
      inline virtual ~number_node(){/* STUB */};
    };

    struct symbol_node : public node {
      int64_t id;
      node_type type = symbol;
      inline virtual ~symbol_node(){/* STUB */};
    };

    struct call_node : public node {
      node *func;
      std::vector<node *> arguments;
      node_type type = call;
      inline virtual ~call_node(){/* STUB */};
    };

    struct string_node : public node {
      cedar::runes str;
      std::vector<node *> arguments;
      node_type type = string;
      inline virtual ~string_node(){/* STUB */};
    };

    struct keyword_node : public node {
      cedar::runes str;
      std::vector<node *> arguments;
      node_type type = keyword;
      inline virtual ~keyword_node(){/* STUB */};
    };


    struct function_node : public node {
      cedar::runes str;
      std::vector<cedar::runes> args;
      node *body;
      node_type type = function;
      inline virtual ~function_node(){/* STUB */};
    };


    struct vector_node : public node {
      std::vector<node *> elems;
      node_type type = vector;
      inline virtual ~vector_node(){/* STUB */};
    };

    struct dict_node : public node {
      std::map<node *, node *> elems;
      node_type type = call;
      inline virtual ~dict_node(){/* STUB */};
    };

    struct do_node : public node {
      std::vector<node *> body;
      node_type type = do_;
      inline virtual ~do_node(){/* STUB */};
    };

    struct def_node : public node {
      uint64_t id = 0;
      bool macro = false;
      bool priv = false;
      node *val;
      node_type type = def;
      inline virtual ~def_node(){/* STUB */};
    };



    // const node is used for unknown types, or the quote operator
    struct const_node : public node {
      ref val;
      node_type type = constant;
      inline virtual ~const_node(){/* STUB */};
    };



    // (. obj <attrs...> [(call ...)])
    // the dot node allows syntax to deeply nest into an object by getting
    // attributes and optionally finally call a method
    // so (. "hello" (len)) or len('hello') in python, expands into
    // - obj = "hello"
    // - attrs = [intern('len')]
    // - has_call = true (call the last attr)
    // - call_args = [] // has an implicit self argument of the form `(len self)`
    // the implicit self is why the call portion exists here. It allows the
    // compiler to optimize the 'self' access and limit lookups.
    struct dot_node : public node {
      node *obj;
      std::vector<uint64_t> attrs;
      bool has_call;
      std::vector<node*> call_args;
      node_type type = dot;
      inline virtual ~dot_node(){/* STUB */};
    };


    struct if_node : public node {
      node *cond;
      node *true_body;
      node *false_body;
      node_type type = if_;
      inline virtual ~if_node(){/* STUB */};
    };




    node *parse(ref, module *);
  }  // namespace ast
}  // namespace cedar

#endif
