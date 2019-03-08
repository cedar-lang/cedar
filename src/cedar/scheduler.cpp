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


#include <cedar/channel.h>
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
#include <cstdlib>
#include <flat_hash_map.hpp>
#include <mutex>
#include <condition_variable>


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
static std::atomic<u64> pending_job_count;

/**
 * a condition variable for which a thread can wait on when it wants work
 * TODO: Determine if it's a bad idea to globalize this, or to have it at all
 *       as work assigning and work stealing can easily replace this
 */
static std::condition_variable job_pending_cv;

/**
 * ncpus is how many worker threads to maintain at any given time.
 */
static unsigned ncpus = 8;

/**
 * the global work queue
 *
 * this is where work goes in order to be distributed among worker threads.
 * when works is here, it means it is not assigned to a worker and it's ready to
 * be ran. The size of this container is always less or equal to pending_jobs
 */
static std::list<job*> work;

/**
 * A lock around the global work queue.
 * TODO find a way to possibly remove a global work queue and have an explicit
 *      "manager" thread that solely works with the global work and distributes
 *      all work across worker threads
 */
static std::mutex work_mutex;


/**
 * the listing of all worker threads
 * TODO: more docs
 */
static std::vector<worker_thread*> worker_threads;



/**
 * time_ms returns the number of milliseconds from the unix epoch. This is
 * useful for timing fibers
 * TODO: remove the need for this entirely by using libuv
 */
static u64 time_ms(void);


static std::mutex jid_mutex;
int next_jid = 0;


void cedar::add_job(fiber *f) {

  static std::atomic<int> next_jid = 0;
  auto jid = next_jid++;

  job *j = new job();
  j->task = f;
  j->create_time = time_ms();
  j->last_ran = 0;
  j->jid = jid;
  // TODO: atomically add the job to the pending job pool
}

/**
 * construct and start a worker thread by creating it and pushing it to
 * the worker pool atomically
 *
 * A worker thread will sit and wait for jobs in the following order:
 *     A) A job is waiting in the local job queue
 *     B) a job to be accessable in the global queue
 *     C) POSSIBLY, steal work from other worker threads.
 */
static void spawn_worker_thread(void) {
  static unsigned next_wid = 0;

  // TODO: get this working for real.

  auto *thd = new worker_thread();
  thd->wid = next_wid++;
  printf("spawn thread %u: %p\n", thd->wid, thd);
  worker_threads.push_back(thd);
}



/**
 * schedule a single job on the caller thread, it's up to the caller to manage
 * where the job goes after the job yields
 */
static bool schedule_job(job *proc) {
  static int sched_time = 2;
  static bool read_env = false;
  static const char *SCHED_TIME_ENV = getenv("CDR_SCHED_TIME");
  if (SCHED_TIME_ENV != nullptr && !read_env) {
    sched_time = atol(SCHED_TIME_ENV);
    if (sched_time < 2)
      throw cedar::make_exception("$CDR_SCHED_TIME must be larger than 2ms");
    read_env = true;
  }

  bool done = false;
  u64 time = time_ms();
  /* check if this process needs to be run or not */
  if (time >= (proc->last_ran + proc->sleep)) {
    run_context ctx;
    /* "context switch" into the job for sched_time milliseconds
     * and return back here, with a modified ctx */
    proc->task->run(&ctx, sched_time);
    /* store the last time of execution in the job. This is an
     * approximation as getting the ms from epoch takes too long */
    proc->last_ran = time + sched_time;
    /* store how long the process should sleep for.
     * TODO: move this into libuv once that is implenented */
    proc->sleep = ctx.sleep_for;
    /* and increment the ticks for this job */
    proc->ticks++;
    done = proc->task->done;
  }
  return done;
}


/**
 * return if all work has been completed or not
 */
bool cedar::all_work_done(void) {
  // if there are no pending jobs, we must be done with all work.
  return pending_job_count == 0;
}



static void init_scheduler(void) {
  ncpus = std::thread::hardware_concurrency();
  static const char *CDRMAXPROC = getenv("CDRMAXPROC");
  if (CDRMAXPROC != nullptr) ncpus = atol(CDRMAXPROC);

  // spawn 'ncpus' worker threads
  for (unsigned i = 0; i < ncpus; i++) spawn_worker_thread();
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
ref cedar::eval_lambda(lambda *fn) {

  // TODO: tack this into the scheduler... somehow

  /* create a stack-local fiber that will be run to completion */
  fiber f(fn);
  return f.run();
}



ref cedar::call_function(lambda *fn, int argc, ref *argv, call_context *ctx) {
  if (fn->code_type == lambda::function_binding_type) {
    return fn->function_binding(argc, argv, ctx);
  }
  fn = fn->copy();
  fn->prime_args(argc, argv);
  return eval_lambda(fn);
}



/**
 * get the time since epoch in ms
 */
static u64 time_ms(void) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}
