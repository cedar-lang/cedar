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

#include <cedar/object.h>
#include <cedar/scheduler.h>
#include <future>

namespace cedar {


  // forward declarations
  class lambda;
  class scheduler;
  namespace vm {
    class machine;
  }


  struct frame {
    frame *caller = nullptr;
    lambda *code;
    int sp;
    int fp;
    u8 *ip;
  };

  class fiber : public object {
   private:
    int stack_size = 0;
    ref *stack = nullptr;
    // the frame list is a linked list, where the first element
    // is the newest call stack and the last frame in the list
    // is the initial function. Nullptr means to return from the
    // fiber and mark it as done
    frame *call_stack = nullptr;

    void adjust_stack(int);


    frame *add_call_frame(lambda *);
    frame *pop_call_frame(void);

   public:
    bool done = false;
    ref return_value = nullptr;
    int fid = 0;

    fiber(lambda *);
    ~fiber(void);

    void print_callstack();

    // run the fiber for at most max_ms miliseconds
    void run(scheduler *sched, run_context *state, int max_ms);

    // run the fiber until it returns, then return the value it yields
    ref run(void);
  };

}  // namespace cedar
