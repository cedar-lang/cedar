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

#pragma once

#include <cedar/ref.h>
#include <functional>
#include <cedar/scheduler.h>

// just forward declare machine
namespace cedar {namespace vm { class machine; }}

namespace cedar {
  using bound_function = std::function<ref(int, ref*, call_context*)>;
  // using native_callback = std::function<void(const function_callback&)>;
}

#define cedar_binding_sig(name) cedar::ref name(int argc, cedar::ref *argv, cedar::call_context *ctx)
#define cedar_binding(name) \
	cedar_binding_sig(name) asm ("_$CDR$" #name); \
	cedar_binding_sig(name)

#define cedar_init_sig() void cedar_module_init(void)
#define cedar_init() cedar_init_sig() asm ("_$CDR-INIT$"); cedar_init_sig()


#define bind_lambda(argc, argv, ctx) [=] (int argc, cedar::ref *argv, cedar::call_context * ctx) -> cedar::ref
