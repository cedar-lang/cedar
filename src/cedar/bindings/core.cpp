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

#include <cedar/jit.h>
#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/vm/binding.h>
#include <uv.h>



using namespace cedar;
void bind_core(void) {

  module *mod = new module("_core");
  mod->def("hash", [=](const function_callback &args) {
    if (args.len() != 1) {
      args.throw_obj(new string("hash requires 1 argument"));
    }
    args.get_return() = (i64)args[0].hash();
  });



  mod->def("go*", [=](const function_callback &args) {
    if (args.len() != 1) {
      args.throw_obj(new string("go requires a lambda as its argument"));
      return;
    }

    lambda *fn = ref_cast<lambda>(args[0]);
    if (fn == nullptr) {
      args.throw_obj(new string("go requires a lambda as its argument"));
      return;
    }

    fiber *c = new fiber(fn->prime());
    add_job(c);
    args.get_return() = c;
  });




  mod->def("enc", [=](const function_callback &args) {
    ref thing = args[0];
    FILE *fp = fopen("enc-test", "wc");
    serializer s(fp);
    s.write(thing);
    fclose(fp);
  });


  mod->def("denc", [=](const function_callback &args) {
    FILE *fp = fopen("enc-test", "r");
    serializer s(fp);
    ref thing = s.read();
    fclose(fp);

    args.get_return() = thing;
  });

  
  /*
  lambda *fn = jit::compile(nullptr, nullptr);
  mod->def("jittest", fn);
  */


  // the name "_core" is used so the core file can include it and spread
  // it into the actual core module
  define_builtin_module("_core", mod);
}




