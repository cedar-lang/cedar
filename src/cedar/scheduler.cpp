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


#include <cedar/event_loop.h>
#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/fiber.h>
#include <cedar/object/lambda.h>
#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/thread.h>
#include <cedar/types.h>
#include <unistd.h>
#include <uv.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <flat_hash_map.hpp>
#include <cedar/jit.h>
#include <mutex>


#define GC_THREADS
#include <gc/gc.h>


using namespace cedar;




/**
 * The number of jobs that have yet to be completed. This number is used
 * to track if the main thread should exit or not, and if a certain thread
 * should wait or not. When a job is pushed to the scheduler, this value is
 * incremented, and is decremented when a job is deemed completed.
 *
 * This number is an unsigned 64 bit integer, because cedar will eventually
 * be able to run a huge number of these jobs concurrently. :)
 */
static std::atomic<i64> jobc;

/**
 * ncpus is how many worker threads to maintain at any given time.
 */
static unsigned ncpus = 8;



/**
 * the listing of all worker threads and a fast lookup map to the same
 * information
 * TODO: more docs
 */
static std::mutex worker_thread_mutex;
static std::vector<worker_thread *> worker_threads;
static std::vector<std::thread> thread_handles;

static thread_local fiber *_current_fiber = nullptr;
static thread_local worker_thread *_current_worker = nullptr;
static thread_local bool _is_worker_thread = false;
static thread_local int sched_depth = 0;




static worker_thread *lookup_or_create_worker() {
  static int next_wid = 0;

  if (_current_worker != nullptr) {
    return _current_worker;
  }


  worker_thread_mutex.lock();

  worker_thread *w;
  w = new worker_thread();
  w->wid = next_wid++;
  worker_threads.push_back(w);
  _current_worker = w;
  worker_thread_mutex.unlock();
  return w;
}


fiber *cedar::current_fiber() { return _current_fiber; }


void cedar::add_job(fiber *f) {
  std::lock_guard guard(worker_thread_mutex);
  int ind = rand() % worker_threads.size();
  worker_threads[ind]->local_queue.push(f);
  f->worker = worker_threads[ind];
  worker_threads[ind]->work_cv.notify_all();
}




void _do_schedule_callback(uv_idle_t *handle) {
  auto *t = static_cast<worker_thread *>(handle->data);
  schedule(t);
  if (t->local_queue.size() == 0) {
    uv_idle_stop(handle);
  }
}




static std::thread spawn_worker_thread(void) {
  return std::thread([](void) -> void {
    register_thread();
    _is_worker_thread = true;
    worker_thread *thd = lookup_or_create_worker();
    thd->internal = true;

    while (true) schedule(thd, true);

    deregister_thread();
    return;
  });
}




/**
 * schedule a single job on the caller thread, it's up to the caller to manage
 * where the job goes after the job yields
 */
void schedule_job(fiber *proc) {
  static int sched_time = 2;
  static bool read_env = false;


  if (!read_env) {
    static const char *SCHED_TIME_ENV = getenv("CDRTIMESLICE");
    if (SCHED_TIME_ENV != nullptr) sched_time = atol(SCHED_TIME_ENV);
    if (sched_time < 2) {
      throw cedar::make_exception("$CDRTIMESLICE must be larger than 2ms");
    }
  }
  read_env = true;

  if (proc == nullptr) return;

  u64 time = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  int state = proc->state.load();

  if (state == SLEEPING) {
    if (time < (proc->last_ran + proc->sleep)) {
      return;
    }
  }

// #define LOG_RUN_TIME
#ifdef LOG_RUN_TIME
  auto start = std::chrono::steady_clock::now();
#endif


  fiber *old_fiber = _current_fiber;
  _current_fiber = proc;
  proc->resume();
  _current_fiber = old_fiber;


#ifdef LOG_RUN_TIME
  auto end = std::chrono::steady_clock::now();
  std::cout << "ran: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     start)
                   .count()
            << " us" << std::endl;
#endif

  /* store the last time of execution in the job. This is an
   * approximation as getting the ms from epoch takes too long */
  proc->last_ran = time + sched_time;
  /* and increment the ticks for this job */
  proc->ticks++;
}


/**
 * return if all work has been completed or not
 */
bool cedar::all_work_done(void) {
  // if there are no pending jobs, we must be done with all work.
  return jobc.load() == 0;
}


static void init_scheduler(void) {

  // the number of worker threads is the number of cpus the host machine
  // has, minus one as the main thread does work
  ncpus = std::thread::hardware_concurrency();
  static const char *CDRMAXPROCS = getenv("CDRMAXPROCS");
  if (CDRMAXPROCS != nullptr) ncpus = atol(CDRMAXPROCS);

  if (ncpus < 1) ncpus = 1;

  // spawn 'ncpus' worker threads
  for (unsigned i = 0; i < ncpus; i++) {
    auto t = spawn_worker_thread();
    t.detach();
  }
}




void cedar::schedule(worker_thread *worker, bool internal_worker) {
  sched_depth++;
  if (worker == nullptr) {
    worker = lookup_or_create_worker();
  }

  bool steal = true;
  bool wait_for_work_cv = true;
  size_t pool_size = 0;
  int i1, i2;
  worker_thread *w1, *w2;

TOP:

  if (false && (current_fiber() != nullptr || sched_depth > 1)) {
    printf("worker: %14p %d  current: %14p   depth: %d\n", worker,
           _is_worker_thread, current_fiber(), sched_depth);
  }

  // work is the thing that will eventually be done, while looking for work,
  // it will be set and checked for equality to nullptr. If at any point it
  // is not nullptr, it will immediately be scheduled.
  fiber *work = nullptr;
  // first check the local queue
  work = worker->local_queue.steal();
  if (work != nullptr) goto SCHEDULE;


  if (steal) {
    worker_thread_mutex.lock();
    // now look through other threads in the thread pool for work to steal
    pool_size = worker_threads.size();
    i1 = rand() % pool_size;
    i2 = rand() % pool_size;
    w1 = worker_threads[i1];
    w2 = worker_threads[i2];
    if (w1->local_queue.size() > w2->local_queue.size()) {
      work = w1->local_queue.steal();
    } else {
      work = w2->local_queue.steal();
    }
    worker_thread_mutex.unlock();
    if (work != nullptr) {
      goto SCHEDULE;
    }
  }

  if (internal_worker) {
    if (wait_for_work_cv) {
      std::unique_lock lk(worker->lock);
      worker->work_cv.wait_for(lk, std::chrono::milliseconds(2));
      lk.unlock();
      goto TOP;
    }
  }

CLEANUP:
  sched_depth--;
  return;

SCHEDULE:

  work->worker = worker;
  worker->ticks++;
  schedule_job(work);

  bool replace = true;

  auto state = work->state.load();

  if (state == STOPPED || state == BLOCKING) replace = false;

  if (state == STOPPED) {
    for (auto &j : work->dependents) {
      add_job(j);
    }
    work->dependents.clear();
  }

  // since we did some work, we should put it back in the local queue
  // but only if it isn't done.
  if (replace) {
    worker->local_queue.push(work);
  }

  if (internal_worker) {
    goto TOP;
  }

  goto CLEANUP;
}




// forward declare this function
namespace cedar {
  void bind_stdlib(void);
};
void init_binding(cedar::vm::machine *m);

/**
 * this is the primary entry point for cedar, this function
 * should be called before ANY code is run.
 */
void cedar::init(void) {
  init_scheduler();
  init_ev();
  type_init();
  init_binding(nullptr);
  bind_stdlib();
  core_mod = require("core");
}


/**
 * eval_lambda is an 'async' evaluator. This means it will add the
 * job to the thread pool with a future-like concept attached, then
 * run the scheduler until the fiber has completed it's work, then
 * return to the caller in C++. It works in this way so that calling
 * cedar functions from within C++ won't fully block the event loop
 * when someone tries to get a mutex lock or something from within
 * such a call
 */
ref cedar::eval_lambda(call_state call) {
  fiber *f = new fiber(call);
  worker_thread *my_worker = lookup_or_create_worker();
  add_job(f);

  // printf("eval_lambda %d\n", sched_depth);

  fiber *cur = current_fiber();
  if (false && cur != nullptr) {
    f->dependents.push_back(cur);
    cur->set_state(fiber_state::BLOCKING);
    yield();
    return f->return_value;
  }
  // my_worker->local_queue.push(&f);
  while (!f->done) {
    schedule(my_worker);
  }
  return f->return_value;
}



ref cedar::call_function(lambda *fn, int argc, ref *argv, call_context *ctx) {
  // printf("%p\n", fn);
  if (fn->code_type == lambda::function_binding_type) {
    function_callback c(fn->self, argc, argv, ctx->coro, ctx->mod);
    fn->call(c);
    return c.get_return();
  }
  return eval_lambda(fn->prime(argc, argv));
}




// yield is just a wrapper around yield nil
void cedar::yield() { cedar::yield(nullptr); }

void cedar::yield(ref val) {
  auto current = current_fiber();
  if (current == nullptr) {
    throw std::logic_error("Attempt to yield without a current fiber failed\n");
  }
  current->yield(val);
}



//////////////////////////////////////////////////////////////////////////////



static void *cedar_unoptimisable;



#define cedar_setsp(x) \
  cedar_unoptimisable = alloca((char *)alloca(sizeof(size_t)) - (char *)(x));



static size_t cedar_stack_size = 4096 * 4;

static size_t get_stack_size(void) { return cedar_stack_size; }



/* Get memory page size. The query is done once. The value is cached. */
static size_t get_page_size(void) {
  static long pgsz = 0;
  if (pgsz != 0) return (size_t)pgsz;
  pgsz = sysconf(_SC_PAGE_SIZE);
  return (size_t)pgsz;
}


static thread_local std::vector<char *> stacks;




void finalizer(void *ptr, void *data) { printf("finalized %p\n", ptr); }

static char *stackalloc() {
  char *stk;



  stk = (char *)GC_MALLOC(get_stack_size());

  return stk;
}


extern "C" void *get_sp(void);
extern "C" void *GC_call_with_stack_base(GC_stack_base_func, void *);
extern "C" int GC_register_my_thread(const struct GC_stack_base *);
extern "C" int GC_unregister_my_thread(void);


coro::coro(std::function<void(coro *)> fn) {
  set_func(fn);
  coro();
}



coro::coro() {
  stk = stackalloc();
  stack_base = stk;
  stack_base += get_stack_size();
}

void coro::set_func(std::function<void(coro *)> fn) { func = fn; }

coro::~coro(void) {
}


bool coro::is_done(void) { return done; }
void coro::yield(void) { yield(nullptr); }

void coro::yield(ref v) {
  value = v;
  if (!setjmp(ctx)) {
    longjmp(return_ctx, 1);
  }
}




ref coro::resume(void) {
  if (done) {
    fprintf(stderr, "Error: Resuming a complete coroutine\n");
    return nullptr;
  }

  if (!setjmp(return_ctx)) {
    if (!initialized) {
      initialized = true;
      // calculate the new stack pointer
      size_t sp = (size_t)(stk);
      sp += get_stack_size();
      sp &= ~15;
      // inline assembly macro to set the stack pointer


      struct GC_stack_base stack_base;
      GC_get_stack_base(&stack_base);
      stack_base.mem_base = (void *)sp;
      GC_register_my_thread(&stack_base);


      cedar_setsp(sp-8);
      func(this);
      done = true;
      // switch back to the calling context
      longjmp(return_ctx, 1);
      return value;
    }
    longjmp(ctx, 1);
    return value;
  }

  return value;
}


















