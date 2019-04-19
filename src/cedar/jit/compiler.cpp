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
#include <cedar/jit.h>
#include <cedar/native_interface.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <cedar/objtype.h>
#include <cedar/parser.h>
#include <cedar/scheduler.h>
#include <mutex>
#include <string>
#include <vector>


using namespace cedar;
using namespace cedar::jit;
using namespace asmjit;
using namespace asmjit::x86;


// static JitRuntime rt;

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
  auto mem = cc.newStack(sizeof(ref), 8, "stack");
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




void jitloop(void) {
  while (true) {
    CodeHolder code;  // Create a CodeHolder.
    code.init(CodeInfo(
        ArchInfo::kTypeHost));  // Initialize it for the host architecture.

    X86Assembler a(&code);  // Create and attach X86Assembler to `code`.

    // Generate a function runnable in both 32-bit and 64-bit architectures:
    bool isX86 = static_cast<bool>(ASMJIT_ARCH_X86);
    bool isWin = static_cast<bool>(ASMJIT_OS_WINDOWS);

    // Signature: 'void func(int* dst, const int* a, const int* b)'.
    X86Gp dst;
    X86Gp src_a;
    X86Gp src_b;

    // Handle the difference between 32-bit and 64-bit calling convention.
    // (arguments passed through stack vs. arguments passed by registers).
    if (isX86) {
      dst = x86::eax;
      src_a = x86::ecx;
      src_b = x86::edx;
      a.mov(dst, x86::dword_ptr(x86::esp, 4));  // Load the destination pointer.
      a.mov(src_a,
            x86::dword_ptr(x86::esp, 8));  // Load the first source pointer.
      a.mov(src_b,
            x86::dword_ptr(x86::esp, 12));  // Load the second source pointer.
    } else {
      dst = isWin ? x86::rcx
                  : x86::rdi;  // First argument  (destination pointer).
      src_a =
          isWin ? x86::rdx : x86::rsi;  // Second argument (source 'a' pointer).
      src_b =
          isWin ? x86::r8 : x86::rdx;  // Third argument  (source 'b' pointer).
    }

    a.movdqu(x86::xmm0, x86::ptr(src_a));  // Load 4 ints from [src_a] to XMM0.
    a.movdqu(x86::xmm1, x86::ptr(src_b));  // Load 4 ints from [src_b] to XMM1.
    a.paddd(x86::xmm0, x86::xmm1);         // Add 4 ints in XMM1 to XMM0.
    a.movdqu(x86::ptr(dst), x86::xmm0);    // Store the result to [dst].
    a.ret();                               // Return from function.
    size_t size = code.getCodeSize();

    auto ptr = mmap(NULL, size, PROT_READ | PROT_EXEC | PROT_WRITE,
                    MAP_ANON | MAP_PRIVATE, -1, 0);
    code.relocate(ptr);  // Relocate & store the output in 'ptr'.

    typedef void (*SumIntsFunc)(int *dst, const int *a, const int *b);

    // Execute the generated function.
    int inA[4] = {4, 3, 2, 1};
    int inB[4] = {1, 5, 2, 8};
    int out[4];

    // This code uses AsmJit's ptr_as_func<> to cast between void* and
    // SumIntsFunc.
    ptr_as_func<SumIntsFunc>(ptr)(out, inA, inB);
    printf("{%d %d %d %d}\n", out[0], out[1], out[2], out[3]);

    // munmap(ptr, size);
    printf("ran %p\n", ptr);
  }
}




lambda *jit::compile(ref obj, module *mod) {
  // jitloop();
  auto ch = new code_handle(obj, mod);
  return new lambda([=](const function_callback &cb) { ch->apply(cb); });
}


lambda *jit::compile(runes s, module *mod) {
  reader reader;
  reader.lex_source(s);
  bool v;
  return compile(reader.read_one(&v), mod);
}


static type *jit_get_type(ref &r) { return r.get_type(); }


static reg get_type(X86Compiler &cc, reg arg) {
  reg ret = cc.newGpz("ret");

  auto call = cc.call(imm_ptr(jit_get_type), FuncSignature1<type *, ref *>());

  call->setArg(0, arg);
  call->setRet(0, ret);
  return ret;
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

void compiler::init(void) {}




void *compiler::run(module *m, size_t *sz) {
  code.init(CodeInfo(ArchInfo::kTypeHost));
#ifdef JIT_DEBUG
  logger.setStream(stdout);
  logger.setIndentation("   ");
  code.setLogger(&logger);
#endif
  cc.~X86Compiler();
  new (&cc) X86Compiler(&code);

  stk = cc.newStack(1, 8, "STACK");
  mod = m;
  auto *root = ast::parse(base_obj, m, nullptr);


  // run analysis on the variables and finalize closure information
  root->sc->finalize();



  cc.addFunc(FuncSignature1<void, function_callback *>());

  cb = cc.newGpz("cb");
  cc.setArg(0, cb);


  auto reta = cc.newGpz("return_value");

  cc.lea(reta, qword_ptr(cb, offsetof(function_callback, return_value)));

  compile_node(reta, root);
  cc.ret();
  cc.finalize();

  size_t size = code.getCodeSize();

  // allocate a region of memory to relocate code to
  auto ptr =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

  code.relocate(ptr);  // Relocate & store the output in 'ptr'.


  mprotect(ptr, size, PROT_EXEC | PROT_READ);

  *sz = size;
  return ptr;
}




#define ISA(obj, t) (dynamic_cast<t *>(obj) != nullptr)
void compiler::compile_node(reg dst, ast::node *obj) {
  if (ISA(obj, number_node)) return compile_number(dst, obj);
  if (ISA(obj, symbol_node)) return compile_symbol(dst, obj);
  if (ISA(obj, call_node)) return compile_call(dst, obj);
  if (ISA(obj, if_node)) return compile_if(dst, obj);

  return compile_nil(dst);
  return;
}



static void _load_ref_int(ref &r, i64 v) { r = v; }
static void _load_ref_float(ref &r, double v) { r = v; }
void compiler::compile_number(reg dst, ast::node *obj) {
  auto n = static_cast<ast::number_node *>(obj);
  COMMENT("NUMBER:");
  if (!n->is_float) {
    reg arg = cc.newI64("num arg");
    debug_reg_print(cc, arg, "arg");
    cc.mov(arg, (int64_t)n->i);

    // load the reference with an int
    auto call =
        cc.call(imm_ptr(_load_ref_int), FuncSignature2<void, ref *, i64>());
    call->setArg(0, dst);
    call->setArg(1, arg);
    return;
  } else {
    auto arg = cc.newXmmSd();
    double d = n->d;
    uint64_t d_re = *(uint64_t *)&d;

    auto itmp = cc.newGpz();
    cc.mov(itmp, d_re);
    cc.movq(arg, itmp);

    // load a reference with a float (double)
    auto call =
        cc.call(imm_ptr(_load_ref_float), FuncSignature2<void, ref *, i64>());
    call->setArg(0, dst);
    call->setArg(1, arg);
    return;
  }
}



void jit_do_call(ref &dst, ref &func, int argc, ref *argv) {
  lambda *fn = ref_cast<lambda>(func);
  if (fn != nullptr) {
    call_context c;
    dst = call_function(fn, argc, argv, &c);
  } else {
    throw make_exception("Unable to call non-lambda ", func);
  }
}



void compiler::compile_call(reg dst, ast::node *obj) {
  auto n = static_cast<ast::call_node *>(obj);
  COMMENT("call:");


  auto func_mem = salloc(sizeof(ref));
  auto func = func_mem.ptr;

  compile_node(func, n->func);


  auto arg_mem = salloc(sizeof(ref) * n->arguments.size());

  auto argc = cc.newGpz("argc");
  cc.mov(argc, (int64_t)n->arguments.size());
  auto argv = cc.newGpz("argv");
  auto arg_dst = cc.newGpz("arg_dst");

  cc.lea(argv, qword_ptr(arg_mem.ptr));


  for (int i = 0; i < n->arguments.size(); i++) {
    cc.lea(arg_dst, qword_ptr(argv, sizeof(ref) * i));
    compile_node(arg_dst, n->arguments[i]);
  }



  auto call = cc.call(imm_ptr(jit_do_call),
                      FuncSignature4<void, ref *, ref *, int, ref *>());
  call->setArg(0, dst);
  call->setArg(1, func);
  call->setArg(2, argc);
  call->setArg(3, argv);

  sfree(func_mem);
  sfree(arg_mem);
}


static int jit_do_bool_convert(ref &val) {
  static ref false_val = new symbol("false");


  if (val.is_nil() || val == false_val) {
    return 0;
  }
  return 1;
}

void compiler::compile_if(reg dst, ast::node *obj) {
  COMMENT("IF:");
  auto n = static_cast<ast::if_node *>(obj);
  auto is_false = cc.newLabel();
  auto br_join = cc.newLabel();
  auto cond_bool = cc.newGpd();


  COMMENT("COND:");
  compile_node(dst, n->cond);

  auto call =
      cc.call(imm_ptr(jit_do_bool_convert), FuncSignature1<int, ref *>());
  call->setArg(0, dst);
  call->setRet(0, cond_bool);

  cc.cmp(cond_bool, 0);
  cc.je(is_false);



  // true
  compile_node(dst, n->true_body);
  cc.jmp(br_join);
  // false
  cc.bind(is_false);
  compile_node(dst, n->false_body);

  // final join node
  cc.bind(br_join);
}


static void jit_do_module_lookup(ref &dst, uint64_t id, module *m) {
  bool f;
  dst = m->find(id, &f);
  if (!f) {
    if (is_global(id)) dst = get_global(id);
  }
}

void compiler::compile_symbol(reg dst, ast::node *obj) {
  auto n = static_cast<ast::symbol_node *>(obj);
  COMMENT("SYMBOL:");
  auto v = n->binding;

  // global lookups should go to the module for lookup
  if (v->global) {
    auto id = cc.newGpz();
    cc.mov(id, v->id);
    auto call = cc.call(imm_ptr(jit_do_module_lookup),
                        FuncSignature3<void, ref *, uint64_t, module *>());

    call->setArg(0, dst);
    call->setArg(1, id);
    call->setArg(2, imm_ptr(mod));
    return;
  }

  printf("STILL NEED TO COMPILE VARS OF TYPE: %s\n", v->str().c_str());
  exit(0);
}

void compiler::compile_nil(reg dst) {
  // the nil reference is just all 0s:
  // copy 8 bytes of ref at a time
  COMMENT("NIL");
  for (int i = 0; i < sizeof(ref); i += 8) {
    cc.mov(qword_ptr(dst, i), 0);
  }
}



static int round_up(int numToRound, int multiple) {
  if (multiple == 0) return numToRound;

  int remainder = numToRound % multiple;
  if (remainder == 0) return numToRound;

  return numToRound + multiple - remainder;
}


stkregion compiler::salloc(short size) {
  size = round_up(size, alignment);
  int region_count = size / alignment;
  stkregion r;
  r.size = size;


  for (int i = 0; i < regions.size(); i++) {
    r.ind = i;
    bool valid = true;
    for (int j = 0; j < region_count; j++) {
      if (regions[i + j]) {
        valid = false;
        break;
      }
    }
    if (valid) goto found;
  }

  r.ind = sbrk(region_count);
  goto found;


found:
  for (int i = 0; i < region_count; i++) {
    regions[r.ind + i] = true;
  }
  r.ptr = cc.newGpz();
  r.used = true;

  cc.lea(r.ptr, stk.adjusted(r.ind * alignment));


#ifdef JIT_DEBUG
  printf("+ %4d bytes: ", r.size);
  dump_stack();
#endif
  return r;
}

void compiler::sfree(stkregion &r) {
  r.used = false;
  int region_count = r.size / alignment;
  int ind = r.ind;

  for (int i = 0; i < region_count; i++) {
    regions[ind + i] = false;
  }

#ifdef JIT_DEBUG
  printf("- %4d bytes: ", r.size);
  dump_stack();
#endif
}

int compiler::sbrk(int rc) {
  int oi = regions.size();
  for (int i = 0; i < rc; i++) {
    regions.push_back(false);
  }
  // resize the stack virtual register
  VirtReg *vr = cc.getVirtRegById(stk.getBaseId());
  vr->_size = regions.size() * alignment;

  return oi;
}


void compiler::dump_stack() {
#ifdef JIT_DEBUG
  printf("stack: {");
  for (int i = 0; i < regions.size(); i++) {
    auto r = regions[i];
    printf("%c", r ? '#' : ' ');
  }
  printf("}\n");
#endif
}


/*

reg compiler::compile_object(ref obj) {
  type *ot = obj.get_type();


#define HANDLE_TYPE(t) \
  if (ot == t##_type) return compile_##t(obj);

  HANDLE_TYPE(number);
  HANDLE_TYPE(list);
  HANDLE_TYPE(symbol);

  throw cedar::make_exception("unable to jit compile object ", obj);
}



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

*/

