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

#include <cedar/runes.h>
#include <utf8.h>
#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>




namespace cedar {
  namespace util {


    inline cedar::runes read_file(char *filename) {
      auto wif = std::ifstream(filename);
      wif.imbue(std::locale());
      std::stringstream wss;
      wss << wif.rdbuf();
      std::string o = wss.str();
      return o;
    }


    inline cedar::runes read_file(const char *filename) {
      return read_file(const_cast<char *>(filename));
    }
    inline cedar::runes read_file(cedar::runes &filename) {
      std::string fn = filename;
      return read_file(fn.c_str());
    }

  }  // namespace util

  template <typename... Args>
  inline void print(Args const &... args) {
    (void)(int[]){0, ((void)(std::cout << args << " "), 0)...};
    putchar('\n');
  }
}  // namespace cedar
