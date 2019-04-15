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
#include <cedar/object/channel.h>
#include <cedar/object/fiber.h>
#include <cedar/object/list.h>
#include <cedar/object/module.h>
#include <cedar/objtype.h>
#include <cedar/thread.h>
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


static frame *alloc_frame(void) { return new frame(); }

static void dispose_frame(frame *f) { delete f; }



static u64 time_microseconds(void) {
  auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}




static std::mutex jid_mutex;
static int next_jid = 0;

fiber::fiber(call_state entry) {
  m_type = fiber_type;

  jid_mutex.lock();
  jid = next_jid++;
  jid_mutex.unlock();
  fiber *self = this;
  co.set_func([self](coro *c) { self->co_run(); });
  add_call_frame(entry);
}



fiber::~fiber(void) {
  delete[] stack;
}




void fiber::adjust_stack(int required) {
  if (required == 0) required = 32;
  if (required >= 0 && (stack_size < required || stack == nullptr)) {
    ref *new_stack = new ref[required];
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



inline frame *fiber::add_call_frame(call_state call) {

  frame *frm = new frame();
  frm->call = call;
  frm->caller = top_frame;
  frm->sp = top_frame == nullptr ? 0 : top_frame->sp;
  frm->ip = call.func->code->code;
  top_frame = frm;
  adjust_stack(frm->sp + call.func->code->stack_size);
  return frm;


  frame &old = frames.back();
  frames.push_back(frame{});
  frame &back = frames.back();
  back.call = call;
  back.sp = top_frame == nullptr ? 0 : old.sp;
  back.ip = call.func->code->code;
  top_frame = &back;
  adjust_stack(back.sp + call.func->code->stack_size);
  return &back;
}



frame *fiber::pop_call_frame(void) {
  frame *frm = top_frame;
  top_frame = top_frame->caller;
  return frm;
}




ref fiber::resume() {
  run(4);
  return return_value;
  return co.resume();
}

void fiber::yield() {
  // just yield nothing
  yield(nullptr);
}

void fiber::yield(ref v) {
  co.done = done;
  co.yield(v);
}



void fiber::co_run(void) {
  while (!done) {
    run(4);
    yield(return_value);
  }
}

// run a fiber for its first return value
// be it a yield or a real return
ref fiber::run(void) {
  run(-1);
  return nullptr;
}




// the primary run loop for fibers in cedar
void fiber::run(int max_ms) {
  state = RUNNING;
  u64 max_time = max_ms * 1000;
  // this function should have very minimal initialization code at the start
  // in order to make the yield operations easier. It should just act on
  u64 start_time = time_microseconds();

  reductions = 2000;


  int sp;
  u8 *ip;

  u8 op = 0;
  u64 ran = 0;
  state.store(RUNNING);
  u64 instructions_per_check = 1000;




#define LOAD_CTX()            \
  if (top_frame != nullptr) { \
    sp = top_frame->sp;       \
    ip = top_frame->ip;       \
  }

#define STORE_CTX()           \
  if (top_frame != nullptr) { \
    top_frame->sp = sp;       \
    top_frame->ip = ip;       \
  }

  LOAD_CTX();


#define PROG() top_frame->call.func
#define LOCALS() top_frame->call.locals

  // #define SP() top_frame->sp
  // #define IP() top_frame->ip

#define PUSH(val) (stack[sp++] = (val))
#define POP() (stack[--sp])
#define CODE_READ(type) (*(type *)(void *)ip)
#define CODE_SKIP(type) (ip += sizeof(type))
#define LABEL(op) DO_##op
#define CONSTANT(i) (PROG()->code->constants[(i)])
#define SET_LABEL(op) threaded_labels[op] = &&DO_##op;
#define TARGET(op) \
  case op:         \
    DO_##op:


#define YIELD() \
  STORE_CTX();  \
  return; \
  yield();


#define OP_UNKNOWN 0xFF

  static bool created_thread_labels = false;
  static void *threaded_labels[255];
  // if the global thread label vector isn't initialized, we need to do that
  // first before executing any bytecode
  if (!created_thread_labels) {
    for (int i = 0; i < 255; i++) threaded_labels[i] = &&LABEL(OP_UNKNOWN);
    SET_LABEL(OP_NOP);
    SET_LABEL(OP_NIL);
    SET_LABEL(OP_CONST);
    SET_LABEL(OP_FLOAT);
    SET_LABEL(OP_INT);
    SET_LABEL(OP_INT_NEG_1);
    SET_LABEL(OP_INT_0);
    SET_LABEL(OP_INT_1);
    SET_LABEL(OP_INT_2);
    SET_LABEL(OP_INT_3);
    SET_LABEL(OP_INT_4);
    SET_LABEL(OP_INT_5);


    SET_LABEL(OP_LOAD_LOCAL);
    SET_LABEL(OP_SET_LOCAL);
    SET_LABEL(OP_LOAD_GLOBAL);
    SET_LABEL(OP_SET_GLOBAL);
    SET_LABEL(OP_SET_PRIVATE);
    SET_LABEL(OP_CONS);
    SET_LABEL(OP_APPEND);
    SET_LABEL(OP_CALL);
    SET_LABEL(OP_MAKE_FUNC);

    SET_LABEL(OP_MAKE_SCOPE);
    SET_LABEL(OP_POP_SCOPE);

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
    SET_LABEL(OP_SUB);
    SET_LABEL(OP_NEG);
    SET_LABEL(OP_INC);
    SET_LABEL(OP_DEC);



    SET_LABEL(OP_LOAD_SELF);
    SET_LABEL(OP_SEND);
    SET_LABEL(OP_RECV);
    SET_LABEL(OP_DICT_SET);
    SET_LABEL(OP_GET_CURRENT_FUNC);
    created_thread_labels = true;
  }




#define PRELUDE \
  if (sp > stack_size - 10) adjust_stack(stack_size + 200);


#define DISPATCH goto loop;



#define PREDICT(NEXTOP)   \
  do {                    \
    u8 next = *ip;        \
    if (next == NEXTOP) { \
      ip++;               \
      goto DO_##NEXTOP;   \
    }                     \
  } while (0);



loop:

  if (ran > instructions_per_check) {
    auto t = time_microseconds();
    auto elapsed = time_microseconds() - start_time;
    ran = 0;
    if (elapsed > max_time) {
      state.store(PARKED);
      start_time = t;
      YIELD();
    }
  }




  // grab the next opcode and increment the instruction pointer
  op = *ip;
  ip++;
  ran++;

  // printf("%02x %p\n", op, top_frame);

  goto *threaded_labels[op];

  switch (op) {
    TARGET(OP_UNKNOWN) {
      PRELUDE;
      fprintf(stderr, "Unhandled instruction 0x%02x in lambda ", op);
      std::cout << PROG()->defining << std::endl;
      exit(-1);
      DISPATCH;
    }


    TARGET(OP_NOP) { DISPATCH; }

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


    TARGET(OP_INT_NEG_1) {
      PRELUDE;
      PUSH(ref{(i64)-1});
      DISPATCH;
    }

    TARGET(OP_INT_0) {
      PRELUDE;
      PUSH(ref{(i64)0});
      DISPATCH;
    }
    TARGET(OP_INT_1) {
      PRELUDE;
      PUSH(ref{(i64)1});
      DISPATCH;
    }
    TARGET(OP_INT_2) {
      PRELUDE;
      PUSH(ref{(i64)2});
      DISPATCH;
    }
    TARGET(OP_INT_3) {
      PRELUDE;
      PUSH(ref{(i64)3});
      DISPATCH;
    }
    TARGET(OP_INT_4) {
      PRELUDE;
      PUSH(ref{(i64)4});
      DISPATCH;
    }
    TARGET(OP_INT_5) {
      PRELUDE;
      PUSH(ref{(i64)5});
      DISPATCH;
    }


    TARGET(OP_LOAD_LOCAL) {
      PRELUDE;
      auto ind = CODE_READ(u8);
      CODE_SKIP(u8);
      auto val = LOCALS()->at(ind);
      PUSH(val);

      DISPATCH;
    }


    TARGET(OP_SET_LOCAL) {
      PRELUDE;
      auto ind = CODE_READ(u8);
      CODE_SKIP(u8);
      LOCALS()->at(ind) = stack[sp - 1];
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
      ref *argv = stack + sp - argc;
      CODE_SKIP(i64);
      i64 new_fp = sp - argc - 1;
      int abp = sp - argc; /* argumement base pointer, represents the base
                              of the argument list */
      if (stack[new_fp].isa(lambda_type)) {
        auto *new_program = stack[new_fp].reinterpret<cedar::lambda *>();
        if (new_program == nullptr) {
          throw cedar::make_exception(
              "Function to be run in call returned nullptr");
        }

        if (new_program->code_type == lambda::bytecode_type) {
          auto call = new_program->prime(argc, stack + abp);


          sp = new_fp;
          STORE_CTX();
          add_call_frame(call);
          LOAD_CTX();

          PREDICT(OP_RETURN);
          PREDICT(OP_SET_GLOBAL);
          DISPATCH;
        } else if (new_program->code_type == lambda::function_binding_type) {
          call_context ctx;
          ctx.coro = this;
          ctx.mod = PROG()->mod;
          function_callback c(PROG()->self, argc, argv, this, PROG()->mod);
          new_program->call(c);
          ref val = c.get_return();
          sp = new_fp;
          PUSH(val);
          PREDICT(OP_RETURN);
          PREDICT(OP_SET_GLOBAL);
          DISPATCH;
        }
      } else if (stack[new_fp].is<type>()) {
        /**
         *
         * create an instance of a type
         *
         */
        static auto __alloc__id = symbol::intern("__alloc__");
        static auto new_id = symbol::intern("new");
        // if the function to be called was a type, we need to make an
        // instance
        type *cls = stack[new_fp].as<type>();
        ref alloc_func_ref = cls->getattr_fast(__alloc__id);
        call_context ctx;
        ctx.coro = this;
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
        sp = new_fp + 1;
        PREDICT(OP_RETURN);
        PREDICT(OP_SET_GLOBAL);
        DISPATCH;
      }
      ref v = self_callv(stack[new_fp], "apply", argc + 1, stack + abp - 1);
      stack[new_fp] = v;
      sp = new_fp + 1;
      DISPATCH;
    }

    TARGET(OP_MAKE_FUNC) {
      PRELUDE;
      auto ind = CODE_READ(u64);
      CODE_SKIP(u64);
      ref function_template = PROG()->code->constants[ind];
      auto *template_ptr = (lambda *)function_template.get();
      lambda *function = template_ptr->copy();
      // inherit closures from parent, a new
      // child closure is created on call
      function->m_closure = LOCALS();
      // when creating functions, inherit the module object
      function->mod = PROG()->mod;
      // inherit the self object as well
      function->self = PROG()->self;
      PUSH(function);
      DISPATCH;
    }


    TARGET(OP_MAKE_SCOPE) {
      PRELUDE;
      auto size = POP().to_int();
      auto ind = POP().to_int();
      top_frame->call.locals = new closure(size, top_frame->call.locals, ind);
      DISPATCH;
    }


    TARGET(OP_POP_SCOPE) {
      PRELUDE;
      top_frame->call.locals = top_frame->call.locals->m_parent;
      DISPATCH;
    }




    TARGET(OP_RETURN) {
      PRELUDE;
      ref val = POP();

      pop_call_frame();

      if (top_frame == nullptr) {
        state.store(STOPPED);
        done = true;
        return_value = val;
        YIELD();
        return;
      }
      LOAD_CTX();

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

      int abp = sp - argc; /* argumement base pointer, represents the base
                              of the argument list */

      while (LOCALS()->func == nullptr) {
        LOCALS() = LOCALS()->m_parent;
      }

      PROG()->set_args_closure(LOCALS(), argc, stack + abp);
      ip = PROG()->code->code;

      sp = top_frame->caller->sp + 1;

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
      auto val = stack[sp - off];
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
      u64 id = CODE_READ(i64);
      CODE_SKIP(i64);
      vm::set_macro(id, func);


      auto s = new symbol();
      s->id = id;



      PUSH(nullptr);
      DISPATCH;

      /* Everything below here is experimental */

      lambda *f = ref_cast<lambda>(func);
      if (f != nullptr) {
        f->macro = true;
        if (PROG()->mod != nullptr) {
          bool found;
          PROG()->mod->find(id, &found);
          if (found) {
            std::cout << "MACRO REASSIGN: " << symbol::unintern(id) << " "
                      << std::endl;
          }
          PROG()->mod->setattr_fast(id, f);
          PUSH(func);
          DISPATCH;
        }
      }

      PUSH(nullptr);
      DISPATCH;
    }


    TARGET(OP_EVAL) {
      PRELUDE;

      ref expr = POP();
      vm::compiler c;
      ref compiled_lambda = c.compile(expr, nullptr);
      lambda *func = compiled_lambda.as<lambda>();
      call_state call = func->prime(0, nullptr);
      fiber eval_fiber(call);
      ref res = eval_fiber.run();
      PUSH(res);
      DISPATCH;
    }


    TARGET(OP_SLEEP) {
      PRELUDE;
      ref dur_ref = POP();
      done = false;
      i64 interval = 0;
      if (dur_ref.get_type() == number_type) {
        interval = dur_ref.to_int();
      }

      sleep = interval;

      state.store(SLEEPING);
      // push a value for when we come back
      PUSH(nullptr);
      YIELD();
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


    TARGET(OP_SUB) {
      PRELUDE;
      ref b = POP();
      ref a = POP();
      PUSH(a - b);
      DISPATCH;
    }


    TARGET(OP_NEG) {
      PRELUDE;
      ref a = POP();
      PUSH(a * -1);
      DISPATCH;
    }

    TARGET(OP_INC) {
      PRELUDE;
      ref a = POP();
      PUSH(a + 1);
      DISPATCH;
    }
    TARGET(OP_DEC) {
      PRELUDE;
      ref a = POP();
      PUSH(a - 1);
      DISPATCH;
    }


    TARGET(OP_LOAD_SELF) {
      PRELUDE;
      PUSH(PROG()->self);
      DISPATCH;
    }


    TARGET(OP_SEND) {
      PRELUDE;

      ref item = POP();
      ref chanr = POP();

      auto *chan = ref_cast<channel>(chanr);
      if (chan == nullptr) {
        throw cedar::make_exception("unable to send on invalid channel: ",
                                    chanr);
      }

      sender s{this, item};
      bool sent = chan->send(s);

      // should return nil
      PUSH(nullptr);

      // if it wasn't sent, yield. It's up to the channel to re-add this fiber
      if (!sent) {
        state.store(BLOCKING);
        YIELD();
      }

      DISPATCH;
    }

    TARGET(OP_RECV) {
      PRELUDE;

      ref chanr = POP();
      auto *chan = ref_cast<channel>(chanr);
      if (chan == nullptr) {
        throw cedar::make_exception("unable to recv on invalid channel: ",
                                    chanr);
      }

      ref *slot = stack + sp++;

      receiver r{this, slot};

      bool received = chan->recv(r);

      if (!received) {
        state.store(BLOCKING);
        YIELD();
      }

      DISPATCH;
    }




    TARGET(OP_DICT_SET) {
      PRELUDE;

      DISPATCH;
    }



    TARGET(OP_GET_CURRENT_FUNC) {
      PRELUDE;
      PUSH(PROG());
      DISPATCH;
    }
  }

  goto loop;


exit:
  done = true;
  return_value = POP();
  state.store(STOPPED);
  YIELD();
}




int fiber::get_state(void) { return state.load(); }
void fiber::set_state(fiber_state s) { state = s; }


void fiber::print_callstack(void) {
  static auto name_id = symbol::intern("*name*");
  printf("Fiber #%d\n", jid);
  int i = 0;
  for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
    if (i == 0) {
      printf("* ");
    } else {
      printf("  ");
    }
    printf("frame #%d: ", i++);
    std::cout << ref(it->call.func);

    std::cout << " in ";
    ref mod_name = it->call.func->mod->getattr_fast(name_id);
    std::cout << mod_name;
    printf("\n");
  }
}
