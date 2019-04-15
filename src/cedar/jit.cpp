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


#include <cedar/jit.h>
#include <cedar/native_interface.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/object/module.h>
#include <cedar/object/symbol.h>
#include <cedar/objtype.h>
#include <cedar/parser.h>
#include <string>



using namespace cedar;
using namespace cedar::jit;
using namespace asmjit;
using namespace asmjit::x86;


// Error handler that just prints the error and lets AsmJit ignore it.
class PrintErrorHandler : public asmjit::ErrorHandler {
 public:
  // Return `true` to set last error to `err`, return `false` to do nothing.
  bool handleError(asmjit::Error err, const char *message,
                   asmjit::CodeEmitter *origin) override {
    fprintf(stderr, "ERROR: %s\n", message);
    return false;
  }
};


static PrintErrorHandler er;

static JitRuntime rt;




// #define JIT_DEBUG
#define JIT_REG_DEBUG

#ifdef JIT_DEBUG
#define COMMENT(m) cc.comment(";; " m)
#else
#define COMMENT(m)
#endif

#define REG_DEBUG(name) debug_reg_print(cc, name, #name);
static void debug_reg_print(X86Compiler &cc, reg r, const char *msg) {
#ifdef JIT_JIT_DEBUG
  COMMENT("++++++ DEBUG ++++++");
  auto call =
      cc.call(imm_ptr(printf),
              FuncSignature3<int, const char *, const char *, uint64_t>());
  call->setArg(0, imm_ptr("[JIT DEBUG] %16x: %s\n"));
  call->setArg(1, r);
  call->setArg(2, imm_ptr(msg));
  COMMENT("------ DEBUG ------");
#endif
}




typedef void (*binary_func)(ref &, ref &, ref &);

// various binary operations on the JIT. This needs to be done because the
// jit cannot access or call virtual methods on a ref
static void jit_do_add(ref &dst, ref &a, ref &b) { dst = a + b; }
static void jit_do_sub(ref &dst, ref &a, ref &b) { dst = a - b; }
static void jit_do_mul(ref &dst, ref &a, ref &b) { dst = a * b; }
static void jit_do_div(ref &dst, ref &a, ref &b) { dst = a / b; }



static reg alloc_ref(X86Compiler &cc) {
  COMMENT("+ alloc_ref");
  auto mem = cc.newStack(sizeof(ref), 4);
  mem.setSize(sizeof(ref));
  auto addr = cc.newIntPtr();
  cc.lea(addr, mem);
  debug_reg_print(cc, addr, "alloc_ref addr");
  COMMENT("- alloc_ref");
  return addr;
}


static reg compile_binary_op(X86Compiler &cc, binary_func func, reg a, reg b) {
  auto res = alloc_ref(cc);
  debug_reg_print(cc, res, "bop res");

  auto call = cc.call(
      imm_ptr(func),  //   Function address or Label.
      FuncSignature3<void, ref *, ref *, ref *>());  //   Function signature.
  call->setArg(0, res);
  call->setArg(1, a);
  call->setArg(2, b);

  return res;
}



static void ref_copy(X86Compiler &cc, reg dst, reg src) {
  COMMENT("+ ref_copy");
  reg buf = cc.newInt64("ref_copy_buf");
  // copy 8 bytes of ref at a time
  for (int i = 0; i < sizeof(ref); i += 8) {
    cc.mov(buf, qword_ptr(src, i));
    cc.mov(qword_ptr(dst, i), buf);
  }
  COMMENT("- ref_copy");
}




static reg get_argc(X86Compiler &cc, reg cb) {
  COMMENT("+ get_argc");
  reg argc = cc.newGpz();
  cc.mov(argc, qword_ptr(cb, offsetof(function_callback, m_argc)));
  COMMENT("-  get_argc");
  return argc;
}


static reg get_argv(X86Compiler &cc, reg cb) {
  reg argv = cc.newGpz();
  cc.mov(argv, qword_ptr(cb, offsetof(function_callback, m_argv)));
  return argv;
}



static reg get_arg(X86Compiler &cc, reg cb, int i) {
  COMMENT("+ get_arg");
  auto argv = get_argv(cc, cb);
  auto arg = cc.newGpz();
  cc.lea(arg, qword_ptr(argv, (uint64_t)sizeof(ref) * i));
  COMMENT("- get_arg");
  return arg;
}



static reg get_return(X86Compiler &cc, reg cb) {
  COMMENT("+ get_return");
  reg r = cc.newGpz();
  cc.lea(r, qword_ptr(cb, offsetof(function_callback, return_value)));
  COMMENT("- get_return");
  return r;
}


static reg get_module(X86Compiler &cc, reg cb) {
  COMMENT("+ get_module");
  reg r = cc.newGpz();
  cc.mov(r, qword_ptr(cb, offsetof(function_callback, m_mod)));
  COMMENT("- get_module");
  return r;
}




typedef void (*jit_function)(const function_callback &);


lambda *jit::compile(ref obj, module *mod) {
  FileLogger logger(stdout);
  logger.setIndentation("   ");

  CodeHolder code;
  code.init(rt.getCodeInfo());
  code.setLogger(&logger);
  X86Compiler cc(&code);

  cc.addFunc(FuncSignature1<void, function_callback *>());

  reg callback = cc.newGpz("function_callback");

  cc.setArg(0, callback);

  reg arg1 = get_arg(cc, callback, 0);
  reg arg2 = get_arg(cc, callback, 1);


  auto mulres = compile_binary_op(cc, jit_do_add, arg1, arg2);

  auto r = get_return(cc, callback);
  ref_copy(cc, r, mulres);



  debug_reg_print(cc, r, "return_value");



  cc.ret();

  cc.finalize();


  jit_function square_ref;
  rt.add(&square_ref, &code);



  // square 4 using function_callback
  ref num1 = 6.28;
  ref num2 = 3.14;

  ref args[] = {num1, num2};

  function_callback c(nullptr, 1, args, nullptr, nullptr);

  // pass in result as reference, and num as reference
  // function will write to res
  square_ref(c);

  std::cout << c.get_return() << std::endl;


  return new lambda(*square_ref);
}




compiler::compiler(ref obj) : cc{&code} {
  base_obj = obj;
  init();
}

compiler::compiler(std::string expr) : cc{&code} {
  reader reader;
  reader.lex_source(expr);
  bool v;
  base_obj = reader.read_one(&v);
  init();
}

void compiler::init(void) {
  logger.setStream(stdout);
  logger.setIndentation("   ");
  code.init(rt.getCodeInfo());
  code.setLogger(&logger);
  cc.~X86Compiler();
  new (&cc) X86Compiler(&code);
}


lambda *compiler::run(module *m) {
  cc.addFunc(FuncSignature1<void, function_callback *>());

  cb = cc.newGpz("cb");
  cc.setArg(0, cb);

  REG_DEBUG(cb);
  reg val = compile_object(base_obj);


  reg ret = get_return(cc, cb);
  ref_copy(cc, ret, val);

  cc.ret();

  cc.finalize();


  jit_function fn;
  rt.add(&fn, &code);

  ref args[] = {};

  function_callback c(nullptr, 1, args, nullptr, nullptr);
  c.m_mod = m;

  fn(c);


  std::cout << c.return_value << std::endl;
  return nullptr;
}




reg compiler::compile_object(ref obj) {
  type *ot = obj.get_type();


#define HANDLE_TYPE(t) \
  if (ot == t##_type) return compile_##t(obj);

  HANDLE_TYPE(number);
  HANDLE_TYPE(list);
  HANDLE_TYPE(symbol);

  throw cedar::make_exception("unable to jit compile object ", obj);
}


static void _load_ref_int(ref &r, i64 v) { r = v; }
static void _load_ref_float(ref &r, double v) { r = v; }

reg compiler::compile_number(ref obj) {
  reg n = alloc_ref(cc);


  if (obj.is_int()) {
    reg arg = cc.newI64("num arg");
    debug_reg_print(cc, arg, "arg");
    cc.mov(arg, (int64_t)obj.to_int());

    // load the reference with an int
    auto call =
        cc.call(imm_ptr(_load_ref_int), FuncSignature2<void, ref *, i64>());
    call->setArg(0, n);
    call->setArg(1, arg);

  } else if (obj.is_flt()) {
    auto arg = cc.newXmmSd();
    double d = obj.to_float();
    uint64_t d_re = *(uint64_t *)&d;

    auto itmp = cc.newGpz();
    cc.mov(itmp, d_re);
    cc.movq(arg, itmp);

    // load a reference with a float (double)
    auto call =
        cc.call(imm_ptr(_load_ref_float), FuncSignature2<void, ref *, i64>());
    call->setArg(0, n);
    call->setArg(1, arg);
  }
  return n;
}




reg compiler::compile_list(ref obj) {
  reg n = alloc_ref(cc);


  auto is_call = [](ref & obj, cedar::runes func_name) -> bool {
    cedar::list *list = ref_cast<cedar::list>(obj);
    if (list)
      if (auto func_name_given = ref_cast<cedar::symbol>(list->first());
          func_name_given) {
        return func_name_given->get_content() == func_name;
      }
    return false;
  };




  return n;
}

static int _do_global_lookup(ref &r, uint64_t id, module *m) {
  if (m == nullptr) {
    r = nullptr;
    return 0;
  }

  bool b;
  r = m->find(id, &b, m);
  return b;
}



reg compiler::compile_symbol(ref obj) {
  reg n = alloc_ref(cc);
  symbol *s = ref_cast<symbol>(obj);
  auto id = s->id;
  auto id_reg = cc.newGpz();
  cc.mov(id_reg, (uint64_t)id);
  auto mod = get_module(cc, cb);

  cc.comment(";; do_global_lookup");

  auto call = cc.call(imm_ptr(_do_global_lookup),
                      FuncSignature3<void, ref *, i64, module *>());
  call->setArg(0, n);
  call->setArg(1, id_reg);
  call->setArg(2, mod);
  return n;
}

