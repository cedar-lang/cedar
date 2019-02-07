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

#include <cedar/importutil.h>

static apathy::Path path_resolve_file(apathy::Path p) {
  if (p.exists()) {
    if (p.is_directory()) {
      return p.append("main.cdr");
    }
    return p;
  }

  auto filename = p.filename();
  auto dir = p.directory();
  dir.append(filename + ".cdr");
  return dir;
}

std::string cedar::path_resolve(std::string search, apathy::Path base) {
  apathy::Path sp = search;
  if (sp.is_absolute()) {
    return path_resolve_file(sp.sanitize()).string();
  }

  while (true) {
    auto c = base;
    sp = c.append(sp);
    auto res = path_resolve_file(sp.absolute().sanitize());
    if (res.exists()) {
      return res.string();
    }

    c = base;
    // if the file didn't exist, walk up the directory tree...
    if (auto p = path_resolve_file(c.append("cdr_pkgs").append(sp));
        p.exists()) {
      return p.string();
    }

    base = base.up();
    if (base == "/") break;
  }

  return path_resolve_file(apathy::Path("/usr/local/lib/")
                               .append(search)
                               .absolute()
                               .sanitize())
      .string();
}