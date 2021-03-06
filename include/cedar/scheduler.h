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

#include <cedar/call_state.h>
#include <cedar/cl_deque.h>
#include <cedar/ref.h>
#include <cedar/types.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <uv.h>
#include <future>
#include <list>
#include <mutex>
#include <thread>

#include <gc/gc.h>
#include <gc/gc_cpp.h>

/*
 * The cedar scheduler's job is to distribute work across several worker threads
 */

namespace cedar {


  class coro {
    char *stk = nullptr;
    char *stack_base = nullptr;
    bool initialized = false;
    ref value;
    void run();

   public:
    jmp_buf ctx;
    jmp_buf return_ctx;
    bool done = false;
    std::function<void(coro *)> func;
    coro(std::function<void(coro *)>);
    coro();
    ~coro(void);


    void set_func(std::function<void(coro *)>);
    void yield(void);
    void yield(ref v);
    bool is_done(void);
    ref resume(void);
  };


  template <typename T>
  class locked_queue {
    std::list<T> m_buffer;
    std::mutex m_lock;

   public:
    inline T pop(void) {
      std::lock_guard lock(m_lock);
      if (m_buffer.size() == 0) return T{};
      T f = m_buffer.front();
      m_buffer.pop_front();
      return f;
    }
    inline T steal(void) {
      std::lock_guard lock(m_lock);
      if (m_buffer.size() == 0) return T{};
      T f = m_buffer.back();
      m_buffer.pop_back();
      return f;
    }
    inline void push(T item) {
      std::lock_guard lock(m_lock);
      m_buffer.push_back(item);
    }
    inline size_t size(void) {
      std::lock_guard lock(m_lock);
      return m_buffer.size();
    }
  };

  // forward declaration
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
    int wid = 0;
    std::thread::id tid;
    std::mutex lock;
    std::condition_variable work_cv;
    cl_deque<fiber *> local_queue;
    uv_loop_t loop;
    uv_idle_t idler;
    fiber *current_fiber = nullptr;
    bool continue_working = true;
    bool internal = false;
  };

  struct internal_worker {
    std::thread thread;
    worker_thread *worker;
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
  void schedule(worker_thread * = nullptr, bool = false);



  // yield to the scheduler when inside a fiber. should be able to be
  // called anywhere within or below the fiber::run's call stack
  void yield();
  void yield(ref);


  fiber *current_fiber();

  ref eval_lambda(call_state);
  ref call_function(lambda *, int argc, ref *argv, call_context *ctx);

}  // namespace cedar
