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
#include <cedar/object/keyword.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/vector.h>
#include <cedar/object/fiber.h>
#include <cedar/objtype.h>
#include <cedar/vm/binding.h>
#include <cedar/scheduler.h>
#include <uv.h>


using namespace cedar;


template <typename T>
type *uv_type_init(std::string name) {
  return nullptr;
}



type *uv_timer_type = nullptr;

class uv_timer_obj : public object {
 public:
  uv_timer_t m_timer;
  inline uv_timer_obj() { m_type = uv_timer_type; }
};


static void _timer_cb(uv_timer_t* handle) {
  auto *cb = (lambda*)handle->data;
  cb = cb->copy();
  cb->prime_args(0, nullptr);
  fiber *f = new fiber(cb);
  add_job(f);
}

static cedar_binding(uv_create_timer) {
  uv_loop_t *loop = ctx->schd->loop;

  if (argc != 3) throw cedar::make_exception("uv.create-timer requires 2 args");

  i64 timeout = argv[0].to_int();
  i64 repeat = argv[1].to_int();
  ref cb_ = argv[2];

  if (cb_.get_type() != lambda_type) {
    throw cedar::make_exception("uv.create-timer: third argument must be callback");
  }

  lambda *cb = ref_cast<lambda>(cb_);

  auto *t = new uv_timer_obj();

  t->m_timer.data = cb;

  uv_timer_init(loop, &t->m_timer);

  uv_timer_start(&t->m_timer, _timer_cb, timeout, repeat);
  return nullptr;
}


void bind_uv(void) {
  module *mod = new module("uv");


  mod->def("create-timer", uv_create_timer);


  define_builtin_module("uv", mod);
}
