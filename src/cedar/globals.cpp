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
#include <cedar/object/lambda.h>
#include <cedar/object/symbol.h>
#include <cedar/ref.h>
#include <sparsepp/spp.h>
#include <flat_hash_map.hpp>
#include <mutex>

using namespace cedar;

static std::mutex g_lock;
static ska::flat_hash_map<u64, ref> globals;



module *cedar::core_mod = nullptr;

bool cedar::is_global(u64 id) {
  g_lock.lock();
  bool is = globals.count(id) == 1;
  g_lock.unlock();
  return is;
}

void cedar::def_global(u64 id, ref val) {
  g_lock.lock();
  globals[id] = val;
  g_lock.unlock();
}



void cedar::def_global(ref k, ref val) { def_global(k.as<symbol>()->id, val); }



void cedar::def_global(runes k, ref val) {
  def_global(symbol::intern(k), val);
}



void cedar::def_global(runes k, bound_function f) {
  u64 id = symbol::intern(k);
  ref func = new lambda(f);
  def_global(id, func);
}

void cedar::def_global(runes k, native_callback f) {
  u64 id = symbol::intern(k);
  ref func = new lambda(f);
  def_global(id, func);
}



ref cedar::get_global(u64 id) {
  std::unique_lock<std::mutex> lock(g_lock);

  if (globals.count(id) != 0) {
    return globals.at(id);
  }
  throw cedar::make_exception("Unable to find global variable ",
                              symbol::unintern(id));
  return nullptr;
}

ref cedar::get_global(ref k) { return get_global(k.as<symbol>()->id); }

ref cedar::get_global(runes k) { return get_global(symbol::intern(k)); }
