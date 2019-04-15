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

#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/dict.h>
#include <cedar/object/fiber.h>
#include <cedar/object/keyword.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/vector.h>
#include <cedar/serialize.h>

#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/vm/binding.h>
#include <uv.h>

#include <math.h>


using namespace cedar;



static native_callback wrap_math_func(const char *name,
                                      double (*func)(double)) {
  return [=](const function_callback &args) -> void {
    if (args.len() == 1) {
      args.get_return() = (double)func(args[0].to_float());
      return;
    }
    std::string msg = name;
    msg += " requires 1 argument";
    args.throw_obj(new string(msg));
  };
}


// probably overkill and going to cause problems...
// TODO dont use this
unsigned int gcd(unsigned int u, unsigned int v) {
  int shift;
  if (u == 0) return v;
  if (v == 0) return u;
  shift = __builtin_ctz(u | v);
  u >>= __builtin_ctz(u);
  do {
    v >>= __builtin_ctz(v);
    if (u > v) {
      unsigned int t = v;
      v = u;
      u = t;
    }
    v = v - u;
  } while (v != 0);
  return u << shift;
}




void bind_math(void) {
  module *mod = new module("math");

  mod->def("e", 2.7182818284590452353602874713527);
  mod->def("pi", 3.141592653589793238462643383279);

  mod->def("gcd", [=](const function_callback &args) -> void {
    if (args.len() != 2) {
      return args.throw_obj(new string("gcd requires 2 arguments"));
    }
    i64 n1 = args[0].to_int();
    i64 n2 = args[1].to_int();
    i64 g = gcd(n1, n2);
    args.get_return() = g;
  });


  mod->def("fact", [=](const function_callback &args) -> void {
    if (args.len() != 1) {
      return args.throw_obj(new string("fact requires 1 argument"));
    }
    double n = args[0].to_float();
    i64 i = 0;
    double f = 1;
    for (i = 1; i <= n; ++i) {
      f *= i;
    }
    args.get_return() = f;
  });




  mod->def("log", [=](const function_callback &args) -> void {
    if (args.len() == 1) {
      args.get_return() = (double)(log(args[0].to_float()));
      return;
    }
    if (args.len() == 2) {
      double base = args[0].to_float();
      double x = args[1].to_float();
      args.get_return() = (double)(log10(x) / log10(base));
      return;
    }
    args.throw_obj(
        new string("math.log requires 1 or 2 arguments. (log base x) or (log "
                   "x) for base e"));
  });




  mod->def("pow", [=](const function_callback &args) -> void {
    if (args.len() == 2) {
      args.get_return() = (double)(pow(args[0].to_float(), args[1].to_float()));
      return;
    }
    args.throw_obj(new string("pow requires 2 arguments"));
  });



  mod->def("ceil", wrap_math_func("ceil", ceil));
  mod->def("round", wrap_math_func("round", round));
  mod->def("floor", wrap_math_func("floor", floor));

  mod->def("exp", wrap_math_func("exp", exp));
  mod->def("abs", wrap_math_func("abs", abs));

  mod->def("cos", wrap_math_func("cos", cos));
  mod->def("sin", wrap_math_func("sin", sin));
  mod->def("tan", wrap_math_func("tan", tan));

  mod->def("acos", wrap_math_func("acos", acos));
  mod->def("asin", wrap_math_func("asin", asin));
  mod->def("atan", wrap_math_func("atan", atan));

  mod->def("cosh", wrap_math_func("cosh", cosh));
  mod->def("sinh", wrap_math_func("sinh", sinh));
  mod->def("tanh", wrap_math_func("tanh", tanh));

  mod->def("sigmoid", [=](const function_callback &args) -> void {
    if (args.len() == 1) {
      args.get_return() = 1 / (1 + exp(-args[0].to_float()));
      return;
    }
    args.throw_obj(new string("sigmoid requires 1 argument"));
  });

  define_builtin_module("math", mod);
}
