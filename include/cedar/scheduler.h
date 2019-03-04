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

#pragma once

#include <cedar/ref.h>
#include <cedar/types.h>
#include <uv.h>
#include <future>
#include <list>
#include <mutex>
#include <thread>

namespace cedar {

  // forward declaration
  class fiber;
  class lambda;
  class scheduler;
  class module;

  // the intro initialization function
  void init(void);
  void add_job(fiber *);

  // a job is a representation of a fiber's state as
  // viewed by the scheduler
  struct job {
    int jid = 0; /* the job id for this job. Simply an enumerated identifier */
    i64 sleep = 0;   /* how long this process should sleep for. This is relative
                        to the last run time */
    u64 last_ran;    /* when the job was last ran, in milliseconds */
    u64 create_time; /* when the job was created in milliseconds */
    int ticks = 0;   /* how many times this job has been switched into */
    fiber *task; /* the actual task that needs to be run. Must never be null */
  };


  struct run_context {
    bool done = false;
    ref value = nullptr;
    i64 sleep_for = 0;
  };


  scheduler *this_scheduler(void);

  // the context that gets passed into a bound_function
  // call in the fiber loop
  struct call_context {
    fiber *coro;
    scheduler *schd;
    module *mod;
  };



  ref call_function(lambda *, int argc, ref *argv, call_context *ctx);

  // a scheduler is the primary controller of a thread's event loop
  // and it maintains control over the fibers running on the thread.
  // it works on a mix between cooperative and preemptive multitasking
  // using a job system and the fiber evaluators in cedar.
  class scheduler {
   public:
    enum run_state {
      paused,
      running,
      stopped,
    };

   private:
    // schedule a job and return true if there are more jobs
    // and return false if there are no more jobs to run
    job *jobs = nullptr;
    std::list<job *> work;
    std::mutex job_mutex;

   public:
    int jobc = 0;
    bool ready = false;
    uv_loop_t *loop;
    run_state state;
    int sid = 0;

    scheduler(void);
    ~scheduler(void);

    bool schedule(void);
    void add_job(fiber *);
    void set_state(run_state);
    bool tick(void);
  };


  // evaluate a lambda to completion. Mainly used in situations
  // where code runs outside the scheduler like macroexpand
  ref eval_lambda(lambda *);

}  // namespace cedar
