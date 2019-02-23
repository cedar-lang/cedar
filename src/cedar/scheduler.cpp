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


#include <cedar/object/fiber.h>
#include <cedar/object/lambda.h>
#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/thread.h>
#include <cedar/types.h>
#include <unistd.h>
#include <uv.h>
#include <cedar/globals.h>
#include <cedar/modules.h>
#include <chrono>
#include <cstdlib>
#include <mutex>

using namespace cedar;



static u64 time_ms(void) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}



static std::mutex sid_lock;
int next_sid;

scheduler::scheduler(void) {
  // TODO: initialize thread-local state
  std::unique_lock<std::mutex> lock(sid_lock);
  sid = next_sid++;
}

scheduler::~scheduler(void) {
  for (job *j = jobs; j != nullptr;) {
    job *c = j;
    j = j->next;
    remove_job(c);
  }
}

static std::mutex jid_mutex;
int next_jid = 0;
static int job_pool_size = 0;
static job *job_pool = nullptr;


// add a fiber to the front of the jobs linked list
void scheduler::add_job(fiber *f) {
  // grab a lock on this scheduler's jobs
  job_mutex.lock();

  job *j = job_pool;
  if (j != nullptr) {
    job_pool_size--;
    job_pool = j->next;
  } else {
    j = new job();
  }


  j->task = f;
  j->next = jobs;

  if (j->next != nullptr) j->next->prev = j;

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
  work.push(j);
  // jobs = j;

  job_mutex.unlock();
}




void scheduler::remove_job(job *j) {
  // decrement the number of jobs in the system
  jobc--;

  // remove the job from the list
  if (j->prev != nullptr) {
    j->prev->next = j->next;
  }

  if (j->next != nullptr) {
    j->next->prev = j->prev;
  }

  if (jobs == j) {
    jobs = jobs->next;
  }


  /*
  // clear out the job
  memset(j, 0, sizeof(job));
  if (job_pool_size < 50) {
    // add it to the job pool
    j->next = job_pool;
    job_pool = j;
    job_pool_size++;
  } else {
    delete j;
  }
  */
  delete j;
}



static std::mutex io_mutex;



bool scheduler::schedule(void) {
  if (state != running) {
    return false;
  }
  // grab a lock on this scheduler's jobs
  // std::unique_lock<std::mutex> lock(job_mutex);

  // get the current time
  u64 time = time_ms();

  job_mutex.lock();

  if (work.empty()) {
    job_mutex.unlock();
    return false;
  }
  job *next_job = work.front();
  work.pop();
  job_mutex.unlock();

  // if there is no next job
  if (next_job == nullptr) return false;

  bool done = false;
  if (time >= (next_job->last_ran + next_job->sleeping_for)) {
    // printf("Scheduling Job %d\n", next_job->jid);
    next_job->run_count++;
    run_context ctx;

    // run the job for 2 ms
    next_job->task->run(this, &ctx, 2);
    next_job->last_ran = time_ms();

    next_job->sleeping_for = ctx.sleep_for;
    done = ctx.done;
  }

  if (done) {
    remove_job(next_job);
  } else {
    job_mutex.lock();
    work.push(next_job);
    job_mutex.unlock();
  }

  return true;
}

int scheduler::run(void) {
  while (schedule())
    ;
  return jobc;
}


int scheduler::run(u64 ms) {
  u64 start = time_ms();

  while (time_ms() - start <= ms) {
    if (!schedule()) break;
  }

  return jobc;
}



void scheduler::set_state(run_state s) {
  job_mutex.lock();
  state = s;
  job_mutex.unlock();
}



struct sched_thread {
  scheduler sched;
  std::thread thread;
};


static int max_procs = 8;
static std::vector<sched_thread> schdulers;

static scheduler primary_scheduler;
std::thread scheduler_thread;



// the callback for when libuv feels like it's okay to
// schedule a fiber run. How nice of it.
void event_idle_schedule(uv_idle_t *handle) {
  auto *sched = (scheduler *)handle->data;
  sched->schedule();
}


namespace cedar {
  void bind_stdlib(void);
};

  void init_binding(cedar::vm::machine *m);

/**
 * this is the primary entry point for cedar, this function
 * should be called before ANY code is run.
 */
void cedar::init(void) {

  type_init();

  init_binding(nullptr);

  bind_stdlib();

  const char *CDRMAXPROCS = getenv("CDRMAXPROCS");
  if (CDRMAXPROCS != nullptr) {
    max_procs = std::stol(CDRMAXPROCS);
  }
  scheduler_thread = std::thread([](void) {
    register_thread();

    // create the scheduler's libuv loop
    primary_scheduler.loop = new uv_loop_t();
    uv_loop_init(primary_scheduler.loop);


    // store the primary scheduler in the loop itself
    primary_scheduler.loop->data = &primary_scheduler;

    // set it to running
    primary_scheduler.set_state(scheduler::running);

    // in libuv, other events need to occur interleaved with
    // fiber evaluation, so we need to use a uv_idle_t event
    // in order to schedule fibers in the libuv idle time.
    uv_idle_t idler;
    idler.data = &primary_scheduler;

    // add the idler to the scheduler's event loop
    uv_idle_init(primary_scheduler.loop, &idler);

    // start the idle scheduler
    uv_idle_start(&idler, event_idle_schedule);

    uv_run(primary_scheduler.loop, UV_RUN_DEFAULT);


    uv_loop_close(primary_scheduler.loop);
    delete primary_scheduler.loop;
  });


  core_mod = require("core");

  scheduler_thread.detach();
}


void cedar::join_scheduler(void) {
  primary_scheduler.set_state(scheduler::stopped);
  if (scheduler_thread.joinable()) {
    printf("joining\n");
    scheduler_thread.join();
    printf("joined\n");
  }
}


void cedar::add_job(fiber *f) { primary_scheduler.add_job(f); }

ref cedar::eval_lambda(lambda *fn) {
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
