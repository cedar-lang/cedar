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

#include <cedar/cl_deque.h>
#include <cedar/ref.h>
#include <cedar/types.h>
#include <cedar/call_state.h>
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
  class lambda;
  class fiber;
  class scheduler;
  class module;

  // the intro initialization function
  void init(void);
  void add_job(fiber *);


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

  /*
   * a worker thread represents a thread that has volunteered some of it's time
   * to the scheduler
   */
  class worker_thread {
   public:
    u64 ticks = 0;
    std::thread::id tid;
    cl_deque<fiber *> local_queue;
  };


  /**
   * the schedule function is the main function that will execute work. It
   * allows a thread to start working on it's own queue, steal from another
   * queue, or immediately return with no work.
   *
   * It has an optional argument representing the worker_thread that is
   * volunteering it's time. If this optional argument is nullptr, it needs to
   * do a hashtable lookup for the thread and create a worker_thread object if
   * it needs to.
   */
  void schedule(worker_thread* = nullptr, bool = false);


  ref eval_lambda(call_state);
  ref call_function(lambda *, int argc, ref *argv, call_context *ctx);

}  // namespace cedar
