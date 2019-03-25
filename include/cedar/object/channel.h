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

#include <cedar/objtype.h>
#include <cedar/ref.h>
#include <cedar/runes.h>
#include <cedar/scheduler.h>
#include <deque>
#include <vector>


namespace cedar {
  // forward declare
  class fiber;

  struct sender {
    fiber *F;
    ref val;
  };

  struct receiver {
    fiber *F;
    ref *slot;
  };

  class channel_buffer {
   private:
    int64_t m_size = 0;
    ref *segment;

   public:
    inline channel_buffer(int64_t sz) : m_size{sz} { segment = new ref[sz]; }
    inline long size() { return m_size; }
    inline ref get(long i) { return segment[i % size()]; }
    void put(int64_t i, ref x) { segment[i % size()] = x; }
  };


  class channel : public object {
    std::mutex lock;
    std::deque<sender> sendq;
    std::deque<receiver> recvq;

   public:
    channel(void) {
      m_type = channel_type;
    }


    // send takes a sender struct and queues it up in the sendq deque
    // internally. If it was able to handle the send immediately, it returns a
    // true. Otherwise it returns a false. This is so that a fiber can know to
    // add itself back to the scheduler queue or not.
    //
    // true -> successfully sent, continue as usual
    // false -> waiting on recv, yield to scheduler
    inline bool send(sender snd) {
      std::unique_lock l(lock);
      // if there are fibers waiting on values, don't add to the buffer and just
      // send directly at them. Then add the fibers to the scheduler.

      // printf("send %zu\n", recvq.size());
      if (!recvq.empty()) {
        // grab the first receiver from the waiter queue
        auto target = recvq.front();
        recvq.pop_front();
        // set the target slot and add the fiber back to the scheduler
        *target.slot = snd.val;
        add_job(target.F);
        return true;
      }
      sendq.push_back(snd);
      return false;
    }


    // recv works the same way as send, if it was able to read a value from the
    // channel, it returns true.
    //
    // true -> successfully recv'd, continue
    // false -> waiting on send, yield to scheduler
    inline bool recv(receiver rec) {
      std::unique_lock l(lock);
      // printf("recv %zu\n", sendq.size());
      if (!sendq.empty()) {
        auto s = sendq.front();
        sendq.pop_front();
        *rec.slot = s.val;
        add_job(s.F);
        return true;
      }
      // add the receiver to the recvq
      recvq.push_back(rec);
      return false;
    }
  };
};  // namespace cedar

