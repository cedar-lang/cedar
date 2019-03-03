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
#include <cedar/object/fiber.h>
#include <cedar/object/lambda.h>
#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/thread.h>
#include <cedar/types.h>
#include <unistd.h>
#include <uv.h>
#include <chrono>
#include <cstdlib>
#include <mutex>

using namespace cedar;

static u64 time_ms(void) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}



static std::mutex sid_lock;
static int next_sid;

scheduler::scheduler(void) {
  sid = next_sid++;
  // create the scheduler's libuv loop
  loop = new uv_loop_t();
  uv_loop_init(loop);
  // store the primary scheduler in the loop itself
  loop->data = this;
}

scheduler::~scheduler(void) {
  // close out of the loop because this scheduler is done
  uv_loop_close(loop);
}

static std::mutex jid_mutex;
int next_jid = 0;


// add a fiber to the front of the jobs linked list
void scheduler::add_job(fiber *f) {
  std::unique_lock<std::mutex> lock(job_mutex);
  // grab a lock on this scheduler's jobs
  job *j = new job();
  j->task = f;
  j->create_time = time_ms();
  j->last_ran = 0;

  // grab a lock to the jid_mutex lock and take the next jid
  // for this job
  jid_mutex.lock();
  j->jid = next_jid++;
  jid_mutex.unlock();

  // increment the number of jobs in the scheduler
  jobc++;
  printf("%zu items\n", work.size());
  work.push(j);
}




bool scheduler::schedule(void) {
  using mutex_lock = std::unique_lock<std::mutex>;
  if (state != running) {
    return false;
  }
  job *proc;
  u64 time = time_ms();


  {
    mutex_lock lock(job_mutex);
    if (work.empty()) {
      job_mutex.unlock();
      return false;
    }
    proc = work.front();
    work.pop();
  }

  bool done = false;

  if (time >= (proc->last_ran + proc->sleeping_for)) {
    printf("scheduling job %d\n", proc->jid);
    proc->run_count++;
    run_context ctx;
    // run the job for 2 ms
    proc->task->run(this, &ctx, 2);
    proc->last_ran = time_ms();
    proc->sleeping_for = ctx.sleep_for;
    done = ctx.done;
    proc->task->done = done;
  }

  job_mutex.lock();
  if (done) {
    jobc--;
  } else {
    work.push(proc);
  }
  job_mutex.unlock();
  return true;
}

/**
 * tick the scheduler once, meaning schedule a fiber then run the UV loop
 * once
 */
bool scheduler::tick(void) {
  schedule();
  // uv_run(loop, UV_RUN_ONCE);
  return true;
}

void scheduler::set_state(run_state s) {
  job_mutex.lock();
  state = s;
  job_mutex.unlock();
}



////
////
////
////
////
////
////

struct sched_thread {
  scheduler sched;
};


static scheduler *schd;

namespace cedar {
  void bind_stdlib(void);
};

static void init_scheduler(void) {
  schd = new scheduler();
  schd->set_state(scheduler::running);
  return;
}

void init_binding(cedar::vm::machine *m);

/**
 * this is the primary entry point for cedar, this function
 * should be called before ANY code is run.
 */
void cedar::init(void) {
  init_scheduler();
  type_init();
  init_binding(nullptr);
  bind_stdlib();
  core_mod = require("core");
}


void cedar::add_job(fiber *f) { schd->add_job(f); }


/**
 * eval_lambda is an 'async' evaluator. This means it will add the
 * job to the thread pool with a future-like concept attached, then
 * run the scheduler until the fiber has completed it's work, then
 * return to the caller in C++. It works in this way so that calling
 * cedar functions from within C++ won't fully block the event loop
 * when someone tries to get a mutex lock or something from within
 * such a call
 */
ref cedar::eval_lambda(lambda *fn) {
  fiber *f = new fiber(fn);
  schd->add_job(f);
  schd->set_state(scheduler::running);
  while (!f->done) {
    printf("here\n");
    schd->schedule();
  }

  printf("done here\n");
  ref v = f->return_value;
  delete f;
  return v;
}



ref cedar::call_function(lambda *fn, int argc, ref *argv, call_context *ctx) {
  if (fn->code_type == lambda::function_binding_type) {
    return fn->function_binding(argc, argv, ctx);
  }
  fn = fn->copy();
  fn->prime_args(argc, argv);
  return eval_lambda(fn);
}
