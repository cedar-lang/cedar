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

/*
 * The cedar scheduler's job is to distribute work across several worker threads
 *
 * It does this by...
 * TODO: Write how it works :)
 */


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


  bool all_work_done(void);

  // the context that gets passed into a bound_function
  // call in the fiber loop
  struct call_context {
    fiber *coro;
    module *mod;
  };

  class worker_thread {
   public:
    std::thread native;
    unsigned wid = 0;
  };


  ref eval_lambda(lambda *);
  ref call_function(lambda *, int argc, ref *argv, call_context *ctx);

}  // namespace cedar
