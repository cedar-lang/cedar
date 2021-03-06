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
#include <cedar/scheduler.h>
#include <cedar/thread.h>
#include <unistd.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

using namespace cedar;

static uv_loop_t *main_loop;
static uv_async_t async;

static std::mutex work_mutex;
std::condition_variable ev_cv;
static std::queue<std::function<void(uv_loop_t *)>> awaiting_work;


static void pull_event_loop_work(void) {
  std::unique_lock<std::mutex> lock(work_mutex);
  while (!awaiting_work.empty()) {
    auto w = awaiting_work.front();
    awaiting_work.pop();
    w(main_loop);
  }
}



void async_callback(uv_async_t *handle) {
  pull_event_loop_work();
}

void cedar::init_ev(void) {
  auto t = std::thread([] {
    register_thread();
    main_loop = new uv_loop_t();
    uv_loop_init(main_loop);
    async.data = main_loop;
    uv_async_init(main_loop, &async, async_callback);
    uv_run(main_loop, UV_RUN_DEFAULT);
    uv_loop_close(main_loop);
    deregister_thread();
  });

  t.detach();
}



void cedar::in_ev(std::function<void(uv_loop_t *)> fn) {
  std::unique_lock<std::mutex> lock(work_mutex);
  awaiting_work.push(fn);
  uv_async_send(&async);
}




void set_timeout_cb(uv_timer_t *handle) {
  auto *j = (fiber *)handle->data;
  add_job(j);
}


void cedar::set_timeout(int time_ms, fiber *j) {
  in_ev([=](uv_loop_t *loop) {
    uv_timer_t *t = new uv_timer_t();
    t->data = j;
    uv_timer_init(loop, t);
    uv_timer_start(t, set_timeout_cb, time_ms, 0);
  });
}

