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
#include <cedar/object/fiber.h>
#include <cedar/object/list.h>
#include <cedar/object/module.h>
#include <cedar/objtype.h>
#include <cedar/vm/compiler.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <gc/gc.h>
#include <unistd.h>
#include <chrono>
#include <mutex>

using namespace cedar;




// the frame pool is a mechanism that attempts to
// limit allocations on call stack changes

static std::mutex frame_pool_lock;

static int frame_pool_size = 0;
static frame *frame_pool = nullptr;


static frame *alloc_frame(void) {
  frame_pool_lock.lock();
  if (frame_pool != nullptr) {
    auto *f = frame_pool;
    // remove the frame from the pool
    frame_pool = frame_pool->caller;
    frame_pool_size--;
    frame_pool_lock.unlock();
    return f;
  }
  frame_pool_lock.unlock();
  return new frame();
}

static void dispose_frame(frame *f) {
  // clear the frame
  memset(f, 0, sizeof(frame));

  frame_pool_lock.lock();
  // set it's next
  f->caller = frame_pool;

  // insert it into the frame pool
  frame_pool = f;

  frame_pool_size++;
  frame_pool_lock.unlock();
}



static u64 time_microseconds(void) {
  auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}



void fiber::adjust_stack(int required) {
  if (required >= 0 && (stack_size < required || stack == nullptr)) {
    auto *new_stack = new ref[required];

    if (stack != nullptr) {
      for (int i = 0; i < stack_size; i++) {
        new_stack[i] = stack[i];
      }
      delete[] stack;
    }
    stack = new_stack;
    stack_size = required;
  }
}



frame *fiber::add_call_frame(lambda *func) {
  if (func->code_type != lambda::lambda_type::bytecode_type) {
    throw cedar::make_exception(
        "Unable to add call frame of non-bytecode lambda");
  }
  frame *frm = alloc_frame();
  frm->code = func;
  frm->caller = call_stack;
  frm->sp = call_stack == nullptr ? 0 : call_stack->sp;
  frm->ip = func->code->code;
  call_stack = frm;

  adjust_stack(frm->sp + func->code->stack_size);
  return frm;
}




frame *fiber::pop_call_frame(void) {
  frame *frm = call_stack;
  call_stack = call_stack->caller;
  return frm;
}



static std::mutex fid_mutex;
static int next_fid = 0;

fiber::fiber(lambda *entry) {
  m_type = fiber_type;
  add_call_frame(entry);


  fid_mutex.lock();
  fid = next_fid++;
  fid_mutex.unlock();
}


fiber::~fiber(void) {}



void fiber::print_callstack(void) {
  static auto name_id = symbol::intern("*name*");
  printf("Fiber #%d\n", fid);
  int i = 0;
  for (frame *f = call_stack; f != nullptr; f = f->caller) {
    if (i == 0) {
      printf("* ");
    } else {
      printf("  ");
    }
    printf("frame #%d: ", i++);
    std::cout << ref(f->code);

    std::cout << " in ";
    ref mod_name = f->code->mod->getattr_fast(name_id);
    std::cout << mod_name;
    printf("\n");
  }
}


// run a fiber for its first return value
// be it a yield or a real return
ref fiber::run(void) {
  run_context ctx;
  run(nullptr, &ctx, -1);
  return ctx.value;
}



static bool created_thread_labels = false;



// the primary run loop for fibers in cedar
void fiber::run(scheduler *sched, run_context *state, int max_ms) {
  u64 max_time = max_ms * 1000;
  // this function should have very minimal initialization code at the start
  // in order to make the yield operations easier. It should just act on
  u64 start_time = time_microseconds();


#define PROG() call_stack->code
#define SP() call_stack->sp
#define IP() call_stack->ip
#define PUSH(val) (stack[SP()++] = (val))
#define POP() (stack[--SP()])

#define CODE_READ(type) (*(type *)(void *)IP())
#define CODE_SKIP(type) (IP() += sizeof(type))

#define LABEL(op) DO_##op
#define TARGET(op) \
  case op:         \
    DO_##op:

#define CONSTANT(i) (PROG()->code->constants[(i)])

#define SET_LABEL(op) threaded_labels[op] = &&DO_##op;

  static void *threaded_labels[255];
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
    SET_LABEL(OP_SET_PRIVATE);
    SET_LABEL(OP_CONS);
    SET_LABEL(OP_APPEND);
    SET_LABEL(OP_CALL);
    SET_LABEL(OP_MAKE_FUNC);
    SET_LABEL(OP_RETURN);
    SET_LABEL(OP_EXIT);
    SET_LABEL(OP_SKIP);
    SET_LABEL(OP_JUMP_IF_FALSE);
    SET_LABEL(OP_JUMP);
    SET_LABEL(OP_RECUR);
    SET_LABEL(OP_DUP);
    SET_LABEL(OP_GET_ATTR);
    SET_LABEL(OP_SET_ATTR);
    SET_LABEL(OP_SWAP);
    SET_LABEL(OP_DEF_MACRO);
    SET_LABEL(OP_EVAL);
    SET_LABEL(OP_SLEEP);
    SET_LABEL(OP_GET_MODULE);
    SET_LABEL(OP_ADD);
    created_thread_labels = true;
  }




#define PRELUDE \
  if (SP() > stack_size - 10) adjust_stack(stack_size * 2)
#define DISPATCH \
  ran++;         \
  goto loop;



#define PREDICT(NEXTOP)   \
  do {                    \
    u8 next = *IP();      \
    if (next == NEXTOP) { \
      IP()++;             \
      goto DO_##NEXTOP;   \
    }                     \
  } while (0);

  // #define PREDICT(NEXTOP)
  u8 op = 0;
  u64 ran = 0;


loop:

  if (max_ms != -1 && sched != nullptr) {
    /* only do a time check every instructions_per_check
     * instructions this is because this loop is *very*
     * performance sensitive and time_microseconds() takes a
     * bit of that speed away
     * TODO: possibly swich from using a time-based scheduler
     *       to using an instruction count based scheduler
     */
    u64 instructions_per_check = 5000;
    if (ran > instructions_per_check) {
      ran = 0;
      u64 current = time_microseconds();
      if (current - start_time > max_time) {
        state->done = false;
        state->value = nullptr;
        return;
      }
    }
  }

  op = *IP();
  IP()++;





  /*
  switch (op) {
#define V(NAME, VAL, a, b)           \
  case VAL:                          \
    printf("%02x: %s\n", op, #NAME); \
    break;
    CEDAR_FOREACH_OPCODE(V)
#undef V
  }
  */

  ran++;

  goto *threaded_labels[op];
  switch (op) {
    TARGET(OP_NOP) {
      PRELUDE;
      fprintf(stderr, "Unhandled instruction %02x in lambda ", op);
      std::cout << PROG()->defining << std::endl;
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
      CODE_SKIP(u64);
      ref val = CONSTANT(ind);
      PUSH(val);
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
      PROG()->m_closure->at(ind) = stack[SP() - 1];
      DISPATCH;
    }


    TARGET(OP_LOAD_GLOBAL) {
      PRELUDE;
      u64 ind = CODE_READ(u64);
      CODE_SKIP(u64);
      ref val = nullptr;

      module *m = PROG()->mod;

      if (m != nullptr) {
        bool has = false;
        ref v = m->find(ind, &has, m);
        if (has) {
          PUSH(v);
          DISPATCH;
        }
      }


      if (core_mod != nullptr) {
        bool has = false;
        ref v = core_mod->find(ind, &has, m);
        if (has) {
          PUSH(v);
          DISPATCH;
        }
      }
      symbol s;
      s.id = ind;
      ref c = &s;
      val = get_global(c);
      PUSH(val);
      DISPATCH;
    }


    TARGET(OP_SET_GLOBAL) {
      PRELUDE;
      auto ind = CODE_READ(i64);
      CODE_SKIP(i64);
      ref val = POP();

      if (PROG()->mod != nullptr) {
        PROG()->mod->setattr_fast(ind, val);
        PUSH(val);
        DISPATCH;
      }
      def_global(ind, val);
      PUSH(val);
      DISPATCH;
    }

    TARGET(OP_SET_PRIVATE) {
      PRELUDE;
      auto ind = CODE_READ(i64);
      CODE_SKIP(i64);
      ref v = POP();
      PROG()->mod->set_private(ind, v);
      PUSH(v);
      DISPATCH;
    }


    TARGET(OP_CONS) {
      PRELUDE;
      auto lst = POP();
      auto val = POP();
      PUSH(new list(val, lst));
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



    TARGET(OP_CALL) {
      PRELUDE;

      i64 argc = CODE_READ(i64);
      ref *argv = stack + SP() - argc;
      CODE_SKIP(i64);
      i64 new_fp = SP() - argc - 1;
      int abp = SP() - argc; /* argumement base pointer, represents the base of
                              the argument list */


      if (stack[new_fp].isa(lambda_type)) {
        auto *new_program = stack[new_fp].reinterpret<cedar::lambda *>();

        if (new_program == nullptr) {
          throw cedar::make_exception(
              "Function to be run in call returned nullptr");
        }

        if (new_program->code_type == lambda::bytecode_type) {
          new_program = new_program->copy();

          new_program->prime_args(argc, stack + abp);

          // otherwise, call it using the builtin frame pointer and stack
          // pointer logic.
          // callheight++;
          SP() = new_fp;
          add_call_frame(new_program);
          PREDICT(OP_RETURN);
          PREDICT(OP_SET_GLOBAL);
          DISPATCH;
        } else if (new_program->code_type == lambda::function_binding_type) {
          call_context ctx;
          ctx.coro = this;
          ctx.schd = sched;
          ctx.mod = PROG()->mod;
          ref val = new_program->function_binding(argc, argv, &ctx);
          SP() = new_fp;
          PUSH(val);

          PREDICT(OP_RETURN);
          PREDICT(OP_SET_GLOBAL);
          DISPATCH;
        }
      } else if (stack[new_fp].is<type>()) {
        static auto __alloc__id = symbol::intern("__alloc__");
        static auto new_id = symbol::intern("new");
        // if the function to be called was a type, we need to make an instance
        type *cls = stack[new_fp].as<type>();
        ref alloc_func_ref = cls->getattr_fast(__alloc__id);

        call_context ctx;
        ctx.coro = this;
        ctx.schd = sched;
        ctx.mod = PROG()->mod;

        // allocate the instance
        ref inst =
            call_function(alloc_func_ref.as<lambda>(), 0, stack + new_fp, &ctx);

        stack[new_fp] = inst;

        ref new_func_ref = inst->getattr_fast(new_id);
        if (!new_func_ref.is<lambda>()) {
          throw cedar::make_exception("`new` method for ", ref{cls},
                                      " is not a function");
        }
        lambda *new_func = new_func_ref.as<lambda>();
        // call the new function on the object
        call_function(new_func, argc + 1, stack + new_fp, &ctx);
        SP() = new_fp + 1;
        PREDICT(OP_RETURN);
        PREDICT(OP_SET_GLOBAL);
        DISPATCH;
      }

      ref v = self_callv(stack[new_fp], "apply", argc + 1, stack + abp - 1);
      stack[new_fp] = v;
      SP() = new_fp + 1;
      DISPATCH;
    }

    TARGET(OP_MAKE_FUNC) {
      PRELUDE;
      auto ind = CODE_READ(u64);
      ref function_template = PROG()->code->constants[ind];
      (void)function_template.to_string(false);
      auto *template_ptr = (lambda *)function_template.get();
      ref function = template_ptr->copy();
      auto *fptr = ref_cast<cedar::lambda>(function);
      // inherit closures from parent, a new
      // child closure is created on call
      fptr->m_closure = PROG()->m_closure;
      // when creating functions, inherit the module object
      fptr->mod = PROG()->mod;
      PUSH(function);
      CODE_SKIP(u64);
      DISPATCH;
    }




    TARGET(OP_RETURN) {
      PRELUDE;
      ref val = POP();


      frame *old_frame = pop_call_frame();
      dispose_frame(old_frame);

      if (call_stack == nullptr) {
        state->value = val;
        state->done = true;
        done = true;
        return_value = val;
        return;
      }

      PUSH(val);
      DISPATCH;
    }




    TARGET(OP_EXIT) {
      PRELUDE;
      goto exit;
      DISPATCH;
    }



    TARGET(OP_SKIP) {
      PRELUDE;
      SP()--;
      DISPATCH;
    }


    TARGET(OP_JUMP_IF_FALSE) {
      PRELUDE;

      static ref false_val = new symbol("false");
      i64 offset = CODE_READ(i64);
      CODE_SKIP(i64);
      auto val = POP();
      if (val.is_nil() || val == false_val) {
        IP() = PROG()->code->code + offset;
      }
      DISPATCH;
    }



    TARGET(OP_JUMP) {
      PRELUDE;
      i64 offset = CODE_READ(i64);
      IP() = PROG()->code->code + offset;
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

      int abp = SP() - argc; /* argumement base pointer, represents the base of
                              the argument list */



      PROG()->set_args_closure(argc, stack + abp);
      IP() = PROG()->code->code;

      SP() = call_stack->caller->sp + 1;
      DISPATCH;
    }

    TARGET(OP_GET_ATTR) {
      PRELUDE;
      u64 id = CODE_READ(u64);
      CODE_SKIP(u64);
      auto val = POP();
      // create a stack allocated symbol
      symbol s;
      // set it's id
      s.id = id;
      // actually getattr
      auto attr = val.getattr(&s);
      // push the value
      PUSH(attr);
      DISPATCH;
    }


    TARGET(OP_SET_ATTR) {
      PRELUDE;
      i64 id = CODE_READ(i64);
      CODE_SKIP(i64);
      auto val = POP();
      auto obj = POP();
      // create a stack allocated symbol
      symbol s;
      // set it's id
      s.id = id;
      // actually getattr
      obj.setattr(&s, val);
      // push the value
      PUSH(val);
      DISPATCH;
    }



    TARGET(OP_DUP) {
      PRELUDE;
      i64 off = CODE_READ(i64);
      auto val = stack[SP() - off];
      CODE_SKIP(i64);
      PUSH(val);
      PREDICT(OP_SWAP);
      DISPATCH;
    }



    TARGET(OP_SWAP) {
      PRELUDE;
      auto a = POP();
      auto b = POP();
      PUSH(a);
      PUSH(b);
      DISPATCH;
    }


    TARGET(OP_DEF_MACRO) {
      PRELUDE;
      auto func = POP();
      int id = CODE_READ(i64);
      CODE_SKIP(i64);
      vm::set_macro(id, func);
      auto s = new symbol();
      s->id = id;
      PUSH(id);
      DISPATCH;
    }


    TARGET(OP_EVAL) {
      PRELUDE;

      ref expr = POP();
      vm::compiler c;
      ref compiled_lambda = c.compile(expr, nullptr);
      lambda *func = compiled_lambda.as<lambda>();
      fiber eval_fiber(func);
      ref res = eval_fiber.run();
      PUSH(res);
      DISPATCH;
    }


    TARGET(OP_SLEEP) {
      PRELUDE;
      ref dur_ref = POP();
      done = false;
      state->value = nullptr;
      state->done = false;
      i64 interval = 0;
      if (dur_ref.get_type() == number_type) {
        interval = dur_ref.to_int();
      }
      state->sleep_for = interval;
      // push a value for when we come back
      PUSH(nullptr);
      return;
      DISPATCH;
    }

    TARGET(OP_GET_MODULE) {
      PRELUDE;
      PUSH(PROG()->mod);
      DISPATCH;
    }



    TARGET(OP_ADD) {
      PRELUDE;
      ref b = POP();
      ref a = POP();
      PUSH(a + b);
      DISPATCH;
    }
  }

  goto loop;


exit:
  done = true;
  state->value = POP();
  state->done = true;
  return;
}
