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
#ifndef _CHANNEL_H
#define _CHANNEL_H

#include <condition_variable>
#include <list>
#include <thread>


namespace cedar {
  template <class item>
  class not_chan {
   private:
    std::list<item> queue;
    std::mutex m;
    std::condition_variable cv;
    bool closed;

   public:
    not_chan() : closed(false) {}
    void close() {
      std::unique_lock<std::mutex> lock(m);
      closed = true;
      cv.notify_all();
    }
    bool is_closed() {
      std::unique_lock<std::mutex> lock(m);
      return closed;
    }
    void put(const item &i) {
      std::unique_lock<std::mutex> lock(m);
      if (closed) throw std::logic_error("put to closed channel");
      queue.push_back(i);
      cv.notify_one();
    }
    bool get(item &out, bool wait = true) {
      std::unique_lock<std::mutex> lock(m);
      if (wait) cv.wait(lock, [&]() { return closed || !queue.empty(); });
      if (queue.empty()) return false;
      out = queue.front();
      queue.pop_front();
      return true;
    }
  };

}  // namespace cedar

#endif
