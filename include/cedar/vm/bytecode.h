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

#include <cedar/exception.hpp>

#include <cedar/ref.h>
#include <cedar/types.h>
#include <memory.h>
#include <stdio.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <cedar/serialize.h>
#include <vector>
#include <mutex>

namespace cedar {

  class object;


  namespace vm {


    // bytecode object
    class bytecode {
     protected:
       friend cedar::serializer;

      // size is how many bytes are written into the code pointer
      // it also determines *where* to write when writing new data
      u64 size = 0;
      // cap is the number of bytes allocated for the code pointer
      u64 cap = 32;
     public:
      i32 stack_size = 0;

      // constants stores values that the bytecode can reference with
      // a simple index into it.
      std::vector<ref> constants;

      // the code pointer is where this instance's bytecode is actually
      // stored. All calls for read interpret a type from this byte vector
      // and calls for write append to it.
      uint8_t *code;

      inline bytecode() {
        cap = 32;
        code = new uint8_t[cap];
      }


      std::mutex calls_lock;
      std::map<std::vector<type *>, int> calls;


      void print(u8 *ip = nullptr);
      ref &get_const(int);
      int push_const(ref);
      void finalize();

      void record_call(int argc, ref *argv);


      template <typename T>
      inline T read(uint64_t i) const {
        return *(T *)(void *)(code + i);
      }

      template <typename T>
      inline uint64_t write_to(uint64_t i, T val) const {
        if (sizeof(T) + i >= cap)
          throw cedar::make_exception("bytecode unable to write ", sizeof(T),
                                      " bytes to index ", i,
                                      ". Out of bounds.");
        *(T *)(void *)(code + i) = val;
        return size;
      }


      template <typename T>
      inline uint64_t write(T val) {
        if (size + sizeof(val) >= cap - 1) {
          uint8_t *new_code = new uint8_t[cap * 2];
          std::memcpy(new_code, code, cap);
          cap *= 2;
          delete code;
          code = new_code;
        }
        uint64_t addr = write_to(size, val);
        size += sizeof(T);
        return addr;
      }



      inline u64 write_op(u8 op) { return write((u8)op); }


      inline u64 write_op(u8 op, u64 arg) {
        auto r = write((u8)op);
        write((u64)arg);
        return r;
      }

      inline uint64_t get_size() { return size; }
      inline uint64_t get_cap() { return cap; }


      template <typename T>
      inline T &at(int i) {
        return *(T *)(void *)(code + i);
      }
    };

  }  // namespace vm
}  // namespace cedar
