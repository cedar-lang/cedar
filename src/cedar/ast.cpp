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
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/objtype.h>
#include <cedar/vm/machine.h>


using namespace cedar;
using namespace cedar::ast;


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
node *ast::parse(ref obj, module *m) {
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
            return parse(expanded, m);
          }
        }
      }

      // wasn't a macro
    }


    // if is the only form of branching in the language
    if (is_call(obj, "if")) {
      ref cond = obj.rest().first();
      ref tr = obj.rest().rest().first();
      ref fl = obj.rest().rest().first(); // could be nil, (rest nil) === nil
      if_node *n = new if_node();
      n->cond = parse(cond, m);
      n->true_body = parse(tr, m);
      n->false_body = parse(fl, m);
      return n;
    }


  }
}
