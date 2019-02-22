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

#include <cedar/globals.h>
#include <cedar/object/string.h>
#include <cedar/object/vector.h>
#include <cedar/objtype.h>
#include <cedar/vm/binding.h>
#include <stdio.h>

using namespace cedar;

static cedar_binding(stringutil_split) {
  if (argc != 2) throw cedar::make_exception("stringutil/split requires 2 args");
  if (argv[1].get_type() != string_type) throw cedar::make_exception("delimiter must be a string");
  if (argv[0].get_type() != string_type) throw cedar::make_exception("target must be a string");
  std::string delim = argv[1].to_string(true);
  std::string s = argv[0].to_string(true);
  ref res = new vector();
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delim)) != std::string::npos) {
    token = s.substr(0, pos);
    res = self_call(res, "put", new string(token));
    s.erase(0, pos + delim.length());
  }
  res = self_call(res, "put", new string(s));
  return res;
}

static cedar_binding(stringutil_replace) {
  if (argc != 3) throw cedar::make_exception("stringutil/replace requires 3 args");

  for (int i = 0; i < argc; i++) {
    if (argv[i].get_type() != string_type) {
      throw cedar::make_exception("stringutil/replace requires all strings");
    }
  }

  std::string target = argv[0].to_string(true);
  std::string what = argv[1].to_string(true);
  std::string with = argv[2].to_string(true);

  int run = what.size();

  size_t pos = 0;
  std::string token;
  while ((pos = target.find(what)) != std::string::npos) {
    token = target.substr(0, pos);
    target.replace(pos, run, with);
  }
  return new string(target);
}

void bind_stringutil(void) {
  def_global("stringutil/split", stringutil_split);
  def_global("stringutil/replace", stringutil_replace);
}
