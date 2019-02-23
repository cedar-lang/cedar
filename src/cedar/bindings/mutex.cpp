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
#include <cedar/objtype.h>
#include <cedar/modules.h>
#include <cedar/object/string.h>
#include <cedar/object/module.h>
#include <cedar/vm/binding.h>
#include <stdio.h>
#include <mutex>

using namespace cedar;

type *mutex_type = nullptr;

class mutex_obj : public object {
 public:
  std::mutex m_mutex;
  inline mutex_obj() { m_type = mutex_type; }
};


void bind_mutex(void) {


  module *mod = new module("_mutex");


  mutex_type = new type("KernelMutex");
  type_init_default_bindings(mutex_type);

  mutex_type->setattr("__alloc__", bind_lambda(argc, argv, machine) { return new mutex_obj(); });
  mutex_type->set_field("new", bind_lambda(argc, argv, machine) { return nullptr; });

  mutex_type->set_field("locked?", bind_lambda(argc, argv, ctx) {
    static ref true_sym = new symbol("true");
    static ref false_sym = new symbol("false");
    mutex_obj *self = argv[0].as<mutex_obj>();
    if (self->m_mutex.try_lock()) {
      self->m_mutex.unlock();
      return false_sym;
    }
    return true_sym;
  });

  mutex_type->set_field("lock!", bind_lambda(argc, argv, ctx) {
    mutex_obj *self = argv[0].as<mutex_obj>();
    self->m_mutex.lock();
    return nullptr;
  });

  mutex_type->set_field("unlock!", bind_lambda(argc, argv, ctx) {
    mutex_obj *self = argv[0].as<mutex_obj>();
    self->m_mutex.unlock();
    return nullptr;
  });

  def_global(new symbol("KernelMutex"), mutex_type);

  mod->def("KernelMutex", mutex_type);
  define_builtin_module("_mutex", mod);
}
