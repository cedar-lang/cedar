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

#include <algorithm>
#include <array>
#include <chrono>
#include <ratio>

#include <cedar/object.h>
#include <cedar/object/dict.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <cedar/object/user_type.h>
#include <cedar/parser.h>
#include <cedar/ref.h>
#include <cedar/types.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <threadpool/thread_pool.h>
#include <unistd.h>
#include <cedar/util.hpp>
#include <thread>
#include <cedar/objtype.h>

#include "../../lib/std.inc.h"

using namespace cedar;

void init_binding(cedar::vm::machine *m);

vm::machine::machine(void) : m_compiler(this) {

  // before creating anything, init the types
  type_init();

  true_value = cedar::new_obj<cedar::symbol>("t");
  bind(true_value, true_value);
  init_binding(this);

  cedar::runes stdsrc;
  for (unsigned int i = 0; i < src_lib_std_inc_cdr_len; i++) {
    stdsrc.push_back(src_lib_std_inc_cdr[i]);
  }
  eval_string(stdsrc);
}

vm::machine::~machine() {
  // delete[] stack;
}

void vm::machine::bind(ref symbol, ref value) {
  u64 hash = symbol.hash();

  i64 global_ind = 0;

  if (global_symbol_lookup_table.find(hash) ==
      global_symbol_lookup_table.end()) {
    // make space in global table
    global_ind = global_table.size();
    var v;
    v.value = nullptr;
    global_table.push_back(v);
  } else {
    global_ind = global_symbol_lookup_table.at(hash);
  }
  global_symbol_lookup_table[hash] = global_ind;
  global_table[global_ind].value = value;
}

void vm::machine::bind(runes name, bound_function f) {
  ref symbol = new_obj<cedar::symbol>(name);
  ref lambda = new_obj<cedar::lambda>(f);
  bind(symbol, lambda);
}

ref vm::machine::find(ref &symbol) {
  u64 hash = symbol.hash();

  if (global_symbol_lookup_table.find(hash) ==
      global_symbol_lookup_table.end()) {
    return nullptr;
  } else {
    return global_table[global_symbol_lookup_table.at(hash)].value;
  }
  return nullptr;
}

ref vm::machine::eval_file(cedar::runes path) {
  cedar::runes src = cedar::util::read_file(path);
  return this->eval_string(src);
}

ref vm::machine::eval_string(cedar::runes expr) {
  static ref mac_ex_sym = new_obj<symbol>("macroexpand");
  cedar::reader reader;
  ref res = nullptr;
  auto top_level = reader.run(expr);

  bool macro_expander_found = false;

  ref macro_expander;
  for (auto &e : top_level) {
    if (!macro_expander_found) macro_expander = find(mac_ex_sym);
    if (!macro_expander.is_nil()) macro_expander_found = true;

    // std::cout << macroexpand(e, this) << std::endl;

    if (macro_expander_found) {
      ref wrapped =
          newlist(macro_expander, newlist(new_obj<symbol>("quote"), e));
      res = eval(eval(wrapped));
    } else {
      res = eval(e);
    }
  }
  return res;

  // if we have a macro expander...
  // create a thread pool... oh no
  thread_pool expansion_pool(8);
  // create a vector of the valid size, to be filled in as things get expanded

  int completed = 0;
  int job_count = top_level.size();

  std::vector<ref> expanded(job_count);

  std::vector<std::future<ref>> jobs(job_count);

  auto *self = this;
  auto pool_job = [macro_expander, self](ref expr) {
    ref wrapped =
        newlist(macro_expander, newlist(new_obj<symbol>("quote"), expr));
    ref val = self->eval(wrapped);
    return val;
  };

  for (int i = 0; i < job_count; i++) {
    jobs[i] = expansion_pool.enqueue(pool_job, top_level[i]);
  }

  while (completed != job_count) {
    for (int i = 0; i < job_count; i++) {
      // if this job is done...
      if (jobs[i].wait_for(std::chrono::milliseconds(10)) ==
          std::future_status::ready) {
        completed++;
        expanded[i] = jobs[i].get();
      }
    }
  }

  return res;
}

// #define VM_TRACE
// #define VM_TRACE_INTERACTIVE
// #define VM_TRACE_INTERACTIVE_AUTO
#define USE_PREDICT

#ifdef VM_TRACE
static const char *instruction_name(uint8_t op) {
  switch (op) {
#define OP_NAME(name, code, op_type, effect) \
  case code:                                 \
    return #name;
    CEDAR_FOREACH_OPCODE(OP_NAME);
#undef OP_NAME
  }
  return "unknown";
}
#endif

static void *threaded_labels[255];
bool created_thread_labels = false;

ref vm::machine::eval(ref obj) {
  try {
    ref compiled_lambda = m_compiler.compile(obj, this);
    lambda *raw_program = ref_cast<cedar::lambda>(compiled_lambda);
    ref val = eval_lambda(raw_program);
    return val;
  } catch (std::exception &e) {
    std::cerr << "Uncaught Exception: " << e.what() << std::endl;
  } catch (cedar::ref e) {
    std::cerr << "Uncaught Exception: " << e << std::endl;
  }
  return nullptr;
}

static std::mutex gil_mtx;

ref vm::machine::eval_lambda(lambda *raw_program) {





  int max_sp = 0;
  // gil_mtx.lock();
  // raw_program->code->print();]
  //

  if (raw_program == nullptr) {
    throw cedar::make_exception("eval_lambda passed null reference to lambda");
  }

  ref return_value;
  i32 stack_size_required = raw_program->code->stack_size + 10;
  u64 stacksize = stack_size_required;
  ref *stack = new ref[stacksize];

  u64 fp, sp;
  fp = sp = 0;

  // ip is the "instruction pointer" and dereferencing
  // it will select the opcode to run
  uint8_t *ip = nullptr;

  // how high the "call stack" is
  u64 callheight = 0;

  // the currently evaluating opcode
  uint8_t op = OP_NOP;

  ref program = nullptr;

  cedar::runes errormsg;

#define PROG() program.reinterpret<cedar::lambda *>()
#define PUSH(val) (stack[sp++] = (val))
#define PUSH_PTR(ptr) (stack[sp++].store_ptr((void *)(ptr)))
#define POP() (stack[--sp])
#define POP_PTR() (stack[--sp].reinterpret<void *>())

#define CODE_READ(type) (*(type *)(void *)ip)
#define CODE_SKIP(type) (ip += sizeof(type))

#define LABEL(op) DO_##op
#define TARGET(op) DO_##op:

  auto check_stack_size_and_resize = [&](void) {
    max_sp = std::max((int)sp, max_sp);

    if ((i64)sp >= (i64)stacksize - stack_size_required) {
      auto new_size = stacksize + stack_size_required * 4;
      auto *new_stack = new ref[new_size];
      for (i64 i = 0; i < (i64)sp; i++) {
        new_stack[i] = stack[i];
      }
      delete[] stack;
      stack = new_stack;
      stacksize = new_size;
    }
  };

  auto print_stack = [&]() {
    i64 height = 40;
    i64 upper_bound = std::min(stacksize, sp);
    i64 lower_bound = std::max((i64)0, (i64)(sp - height));
    printf("---------------------------------------\n");
    printf("       size: %lu refs\n", stacksize);
    printf("             %lu bytes\n", stacksize * sizeof(ref));
    printf("---------------------------------------\n");
    for (i64 i = lower_bound; i <= upper_bound; i++) {
      if (i == (i64)fp) {
        printf(" fp -> ");
      } else if (i == (i64)sp) {
        printf(" sp -> ");
      } else {
        printf("       ");
      }
      printf("%04lx ", i);
      std::cerr << stack[i];
      std::cerr << std::endl;
    }
    printf("---------------------------------------\n");
  };

#define DEFAULT_PRELUDE \
  check_stack_size_and_resize();

#ifdef VM_TRACE

  std::chrono::high_resolution_clock::time_point last_time =
      std::chrono::high_resolution_clock::now();

  auto trace_current_state = [&](void) {

#ifdef VM_TRACE_INTERACTIVE
    printf("\033[2J");
    printf("\033[2H");
#endif
    std::chrono::high_resolution_clock::time_point now =
        std::chrono::high_resolution_clock::now();
    std::chrono::duration<long long, std::nano> dt = now - last_time;
    double dtns = dt.count();

    printf("op: %02x %-20.15s ", op, instruction_name(op));
    printf("Δt: %5.5fms  ", dtns / 1000.0 / 1000.0);
    printf("sp: %6lu  ", sp);
    printf("depth: %6lu  ", callheight);
    printf("ip: %p ", ip);

    std::cout << std::this_thread::get_id() << " ";
    printf("\n");

    last_time = now;
#ifdef VM_TRACE_INTERACTIVE
    print_stack();
#ifdef VM_TRACE_INTERACTIVE_AUTO
    usleep(10000);
#else
    std::cout << "Press Enter to Continue ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
#endif

#endif
  };

#define PRELUDE    \
  DEFAULT_PRELUDE; \
  trace_current_state();
/*
#define PRELUDE \
        std::chrono::high_resolution_clock::time_point now =
std::chrono::high_resolution_clock::now();  \
        std::chrono::duration<long long, std::nano> dt = now - last_time; \
        double dtns = dt.count();                                         \
        printf("sp: %3zu, fp: %3zu, ip %p, preds: %4ld, prev_dt: %8.5fms, op:
%s\n", (size_t)sp, (size_t)fp, ip, correct_predictions, dtns / 1000.0 / 1000.0,
instruction_name(op)); \ last_time = now;
        */
#else
#define PRELUDE DEFAULT_PRELUDE
#endif
#define DISPATCH goto loop;

  // some opcodes come in very common pairs, making it very possible to do
  // speculative exec on what the next opcode could be... This macro allows high
  // speed assumptions to be made at the end of an opcode that is typically
  // followed by another opcode The performance loss by this check is minimal on
  // modern CPUs because of the l1 cache typically caching 8 bytes, so the best
  // case scenario is a cache miss 1/8 opcodes that predict the next instruction
  // long correct_predictions = 0;
#ifdef USE_PREDICT
#define PREDICT(pred, label) \
  do {                       \
    if (*ip == pred) {       \
      ip++;                  \
      op = *ip;              \
      correct_predictions++; \
      goto label;            \
    } else                   \
      correct_predictions--; \
  } while (0);

#else
#define PREDICT(pred, label)
#endif

#define SET_LABEL(op) threaded_labels[op] = &&DO_##op;

  // if the global thread label vector isn't initialized, we need to do that
  // first
  if (!created_thread_labels) {
    for (int i = 0; i < 255; i++) threaded_labels[i] = &&LABEL(OP_NOP);
    SET_LABEL(OP_NIL);
    SET_LABEL(OP_CONST);
    SET_LABEL(OP_FLOAT);
    SET_LABEL(OP_INT);
    SET_LABEL(OP_LOAD_LOCAL);
    SET_LABEL(OP_SET_LOCAL);
    SET_LABEL(OP_LOAD_GLOBAL);
    SET_LABEL(OP_SET_GLOBAL);
    SET_LABEL(OP_CONS);
    SET_LABEL(OP_APPEND);
    SET_LABEL(OP_CALL);
    SET_LABEL(OP_CALL_EXCEPTIONAL);
    SET_LABEL(OP_MAKE_FUNC);
    SET_LABEL(OP_ARG_POP);
    SET_LABEL(OP_RETURN);
    SET_LABEL(OP_EXIT);
    SET_LABEL(OP_SKIP);
    SET_LABEL(OP_JUMP_IF_FALSE);
    SET_LABEL(OP_JUMP);
    SET_LABEL(OP_RECUR);
    SET_LABEL(OP_EVAL);
    created_thread_labels = true;
  }

  program = raw_program;  // ref_cast<cedar::lambda>(progref);

  // PROG()->closure = std::make_shared<closure>(0, nullptr, 0);

  // obtain a reference to the code
  // and set the instruction pointer to the begining of this code.
  ip = PROG()->code->code;

  // push the current code being evaluated. This will also be kept in a local
  // variable in the evaluation loop for quick, uninterupted access without
  // having to go through a slow dynamic_cast() for each call
  PUSH(program);
  // after pushing the lambda to the stack, we need to push the previous frame
  // pointer, which in this case is zero again
  PUSH_PTR(fp);
  // the final thing to push is the previous instruction pointer, and since this
  // is a top level evaluation, pushing nullptr is the only value that really
  // makes sense...
  PUSH(nullptr);

loop:

  // read the opcode from the instruction pointer
  op = *ip;

  ip++;
  goto *threaded_labels[op];

  // OP_NOP is currently a catchall. If this instruction is
  // encountered, an error is printed and the program just exits...
  // TODO: make it not do this and actually just ignore it.
  TARGET(OP_NOP) {
    PRELUDE;
    fprintf(stderr, "UNHANDLED INSTRUCTION: %02x\n", op);
    exit(-1);
    DISPATCH;
  }

  TARGET(OP_NIL) {
    PRELUDE;
    PUSH(nullptr);
    DISPATCH;
  }


  TARGET(OP_CONST) {
    PRELUDE;
    const auto ind = CODE_READ(u64);
    PUSH(PROG()->code->constants[ind]);
    CODE_SKIP(u64);
    DISPATCH;
  }

  TARGET(OP_FLOAT) {
    PRELUDE;
    const auto flt = CODE_READ(double);
    PUSH(flt);
    CODE_SKIP(double);
    DISPATCH;
  }

  TARGET(OP_INT) {
    PRELUDE;
    const auto integer = CODE_READ(i64);
    PUSH(ref{(i64)integer});
    CODE_SKIP(i64);
    DISPATCH;
  }

  TARGET(OP_LOAD_LOCAL) {
    PRELUDE;
    auto ind = CODE_READ(u64);
    auto val = PROG()->m_closure->at(ind);
    PUSH(val);

    CODE_SKIP(u64);
    DISPATCH;
  }

  TARGET(OP_SET_LOCAL) {
    PRELUDE;
    auto ind = CODE_READ(u64);
    CODE_SKIP(u64);
    // ref val = POP();
    PROG()->m_closure->at(ind) = stack[sp - 1];
    // PUSH(val);
    DISPATCH;
  }

  TARGET(OP_LOAD_GLOBAL) {
    PRELUDE;
    i64 ind = CODE_READ(i64);
    CODE_SKIP(i64);
    PUSH(global_table[ind].value);
    DISPATCH;
  }

  TARGET(OP_SET_GLOBAL) {
    PRELUDE;
    auto ind = CODE_READ(i64);
    ref val = POP();
    global_table[ind].value = val;

    PUSH(val);
    CODE_SKIP(i64);
    DISPATCH;
  }

  TARGET(OP_CONS) {
    PRELUDE;

    auto lst = POP();
    auto val = POP();

    auto list = new cedar::list(lst, val);
    PUSH(list);
    DISPATCH;
  }

  TARGET(OP_APPEND) {
    PRELUDE;
    auto a = POP();
    auto b = POP();

    ref r = append(a, b);

    PUSH(r);
    DISPATCH;
  }

  TARGET(OP_CALL)
  TARGET(OP_CALL_EXCEPTIONAL) {
    PRELUDE;
    i64 argc = CODE_READ(i64);
    ref *argv = stack + sp - argc;

    CODE_SKIP(i64);

    i64 new_fp = sp - argc - 1;
    int abp = sp - argc; /* argumement base pointer, represents the base of the
                            argument list */

    ref catcher = nullptr;
    bool is_exceptional = op == OP_CALL_EXCEPTIONAL;
    if (is_exceptional) {
      catcher = stack[new_fp - 1];
      if (!catcher.is<lambda>()) {
        throw cedar::make_exception("exception catcher is not a lambda: ",
                                    catcher);
      }
    }

    try {
      if (stack[new_fp].is<lambda>()) {
        auto *new_program = stack[new_fp].reinterpret<cedar::lambda *>();

        if (new_program == nullptr) {
          errormsg = "Function to be run in call returned nullptr";
          goto error;
        }

        if (new_program->code_type == lambda::bytecode_type) {
          new_program = new_program->copy();

          new_program->prime_args(argc, stack + abp);

          // if the lambda call should be a new call on the c stack, call it
          // that way
          if (true || is_exceptional) {
            ref ret_val = eval_lambda(new_program);
            stack[new_fp] = ret_val;
            sp = new_fp + 1;
            DISPATCH;
          }

          // otherwise, call it using the builtin frame pointer and stack
          // pointer logic.
          callheight++;
          sp = abp;
          PUSH_PTR(ip);
          PUSH_PTR(fp);
          fp = new_fp;
          stack_size_required = new_program->code->stack_size;
          ip = new_program->code->code;
          stack[fp] = new_program;
          program = new_program;
          DISPATCH;
        } else if (new_program->code_type == lambda::function_binding_type) {
          // gil_mtx.unlock();
          ref val = new_program->function_binding(argc, argv, this);
          // gil_mtx.lock();
          sp = abp - 1;

          /* delete all the arguments from the stack by setting them to nullptr
           */
          for (int i = 0; i < argc; i++) {
            stack[abp + i] = nullptr;
          }
          PUSH(val);
          DISPATCH;
        }
      } else if (stack[new_fp].is<user_type>()) {
        user_type *cls = stack[new_fp].reinterpret<user_type *>();
        ref instance = cls->instantiate(argc, stack + abp, this);
        PUSH(instance);
        DISPATCH;
      }
    } catch (ref e) {
      if (is_exceptional) {
        lambda *c = ref_cast<lambda>(catcher);
        c = c->copy();
        c->prime_args(1, &e);
        PUSH(eval_lambda(c));
        DISPATCH;
      } else {
        throw;
      }
    } catch (std::exception &c_exception) {
      if (is_exceptional) {
        ref e = new_obj<string>(runes(c_exception.what()));
        lambda *c = ref_cast<lambda>(catcher);
        c = c->copy();
        c->prime_args(1, &e);
        PUSH(eval_lambda(c));
        DISPATCH;
      } else {
        throw;
      }
    }

    throw cedar::make_exception(
        "function to be called is not a valid callable: ", stack[new_fp]);
    goto error;
  }

  TARGET(OP_MAKE_FUNC) {
    PRELUDE;
    auto ind = CODE_READ(u64);
    ref function_template = PROG()->code->constants[ind];
    auto *template_ptr = ref_cast<cedar::lambda>(function_template);

    ref function = template_ptr->copy();

    auto *fptr = ref_cast<cedar::lambda>(function);

    // inherit closures from parent, a new
    // child closure is created on call
    fptr->m_closure = PROG()->m_closure;

    PUSH(function);
    CODE_SKIP(u64);
    DISPATCH;
  }

  TARGET(OP_ARG_POP) {
    PRELUDE;
    i32 ind = CODE_READ(u64);
    ref arg = stack[fp].first();
    PROG()->m_closure->at(ind) = arg;
    stack[fp] = stack[fp].rest();
    CODE_SKIP(u64);
    DISPATCH;
  }

  TARGET(OP_RETURN) {
    PRELUDE;
    ref val = POP();
    auto prevfp = fp;

    for (auto i = sp; i < fp + stack_size_required; i++) {
      stack[i] = nullptr;
    }
    fp = (u64)POP_PTR();
    ip = (u8 *)POP_PTR();
    sp = prevfp;
    program = ref_cast<cedar::lambda>(stack[fp]);
    stack_size_required = PROG()->code->stack_size;
    callheight--;
    PUSH(val);
    if (ip == 0) goto end;
    DISPATCH;
  }

  TARGET(OP_EXIT) {
    PRELUDE;
    goto end;
    DISPATCH;
  }

  TARGET(OP_SKIP) {
    PRELUDE;
    sp--;
    DISPATCH;
  }

  TARGET(OP_JUMP_IF_FALSE) {
    PRELUDE;

    static ref false_val = new symbol("false");
    i64 offset = CODE_READ(i64);
    CODE_SKIP(i64);
    auto val = POP();
    if (val.is_nil() || val == false_val) {
      ip = PROG()->code->code + offset;
    }
    DISPATCH;
  }

  TARGET(OP_JUMP) {
    PRELUDE;
    i64 offset = CODE_READ(i64);
    ip = PROG()->code->code + offset;
    DISPATCH;
  }

  TARGET(OP_RECUR) {
    PRELUDE;

    i64 argc = CODE_READ(i64);
    CODE_SKIP(i64);
    if (argc != PROG()->argc)
      throw cedar::make_exception(
          "recur call has invalid number of arguments. Given ", argc,
          " expected ", PROG()->argc);

    int abp = sp - argc; /* argumement base pointer, represents the base of the
                            argument list */

    for (int i = 0; i < argc; i++) {
      PROG()->m_closure->at(i + PROG()->arg_index) = stack[abp + i];
      stack[abp + i] = nullptr;
    }
    ip = PROG()->code->code;

    sp = fp + 3;
    DISPATCH;
  }

  TARGET(OP_EVAL) {
    PRELUDE;
    ref thing = POP();
    // gil_mtx.unlock();
    ref res = eval(thing);
    // gil_mtx.lock();
    PUSH(res);
    DISPATCH;
  }

  goto loop;

end:

  return_value = POP();
  delete[] stack;

  return return_value;

error:
  std::cerr << "FATAL ERROR IN EVALUATION:\n";
  std::cerr << " err: " << errormsg << std::endl;
  std::cerr << " stack trace:\n";
  print_stack();

  exit(1);
}
