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
  // TODO: initialize thread-local state
  sid = next_sid++;
}

scheduler::~scheduler(void) {
}

static std::mutex jid_mutex;
int next_jid = 0;
static int job_pool_size = 0;
static job *job_pool = nullptr;


// add a fiber to the front of the jobs linked list
void scheduler::add_job(fiber *f) {
  // grab a lock on this scheduler's jobs
  job *j = new job();
  j->task = f;
  // grab a lock to the jid_mutex lock and take the next jid
  // for this job
  jid_mutex.lock();
  j->jid = next_jid++;
  jid_mutex.unlock();

  j->create_time = time_ms();
  // the job hasn't ran yet, so we just want to run it as
  // soon as possible (give long last_ran time)
  j->last_ran = 0;
  // increment the number of jobs in the scheduler
  jobc++;
  job_mutex.lock();
  work.push(j);
  job_mutex.unlock();
}




void scheduler::remove_job(job *j) {
  // decrement the number of jobs in the system
  jobc--;

  /*
  if (jobs == j) {
    jobs = jobs->next;
  }
  */
  delete j;
}




bool scheduler::schedule(void) {
  if (state != running) {
    return false;
  }
  u64 time = time_ms();
  job_mutex.lock();
  if (work.empty()) {
    job_mutex.unlock();
    return false;
  }
  job *job = work.front();
  work.pop();
  job_mutex.unlock();

  bool done = false;

  if (time >= (job->last_ran + job->sleeping_for)) {
    // printf("Scheduling Job %d %d\n", job->jid, jobc);
    job->run_count++;
    run_context ctx;

    // run the job for 2 ms
    job->task->run(this, &ctx, 2);
    job->last_ran = time_ms();


    job->sleeping_for = ctx.sleep_for;
    done = ctx.done;
    job->task->done = done;
  }



  job_mutex.lock();
  if (done) {
    remove_job(job);
  } else {
    work.push(job);
  }
  job_mutex.unlock();


  return true;
}

/**
 * tick the scheduler once, meaning schedule a fiber then run the UV loop
 * once
 */
bool scheduler::tick(void) {
  // std::cout << "TICK FROM: " << std::this_thread::get_id() << std::endl;
  schedule();
  uv_run(loop, UV_RUN_ONCE);
  return true;
}

void scheduler::set_state(run_state s) {
  job_mutex.lock();
  state = s;
  job_mutex.unlock();
}

bool scheduler::same_thread(void) {
  return m_thread.get_id() == std::this_thread::get_id();
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


static scheduler *primary_scheduler;
std::thread scheduler_thread;
std::thread::id sched_thread;


// the callback for when libuv feels like it's okay to
// schedule a fiber run. How nice of it.
void event_idle_schedule(uv_idle_t *handle) {
  auto *sched = (scheduler *)handle->data;
  sched->schedule();
}

namespace cedar {
  void bind_stdlib(void);
};


static void init_scheduler(void) {
  primary_scheduler = new scheduler();
  scheduler_thread = std::thread([](void) {
    register_thread();
    // create the scheduler's libuv loop
    primary_scheduler->loop = new uv_loop_t();
    uv_loop_init(primary_scheduler->loop);
    // store the primary scheduler in the loop itself
    primary_scheduler->loop->data = &primary_scheduler;
    // set it to running
    primary_scheduler->set_state(scheduler::running);

    while (true) {
      bool has_jobs = primary_scheduler->tick();
      if (!has_jobs) break;
    }

    uv_loop_close(primary_scheduler->loop);
    // uv_run(primary_scheduler.loop, UV_RUN_DEFAULT);

    delete primary_scheduler->loop;
    deregister_thread();
  });

  primary_scheduler->set_thread(std::move(scheduler_thread));



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




void cedar::run_loop(void) {
  while (true) {
    primary_scheduler->tick();
  }
}



void cedar::add_job(fiber *f) { primary_scheduler->add_job(f); }


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


  scheduler s;
  s.set_state(scheduler::running);
  fiber r(fn);
  s.add_job(&r);

  while (!r.done) {
    s.schedule();
  }

  return r.return_value;


  primary_scheduler->add_job(&r);

  while(!r.done) {
    if (primary_scheduler->same_thread()) {
      primary_scheduler->schedule();
    } else {
      // otherwise, sit and wait
      usleep(100);
    }
  }
  return r.return_value;
}



ref cedar::call_function(lambda *fn, int argc, ref *argv, call_context *ctx) {
  if (fn->code_type == lambda::function_binding_type) {
    return fn->function_binding(argc, argv, ctx);
  }
  fn = fn->copy();
  fn->prime_args(argc, argv);
  return eval_lambda(fn);
}
