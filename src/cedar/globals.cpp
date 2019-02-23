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
#include <mutex>
#include <unordered_map>

#include <sparsepp/spp.h>

using namespace cedar;

static std::mutex g_lock;
static std::unordered_map<int, ref> globals;



module *cedar::core_mod = nullptr;


void cedar::def_global(int id, ref val) {
  g_lock.lock();
  globals[id] = val;
  g_lock.unlock();
}



void cedar::def_global(ref k, ref val) {
  def_global(k.as<symbol>()->id, val);
}



void cedar::def_global(runes k, ref val) {
  def_global(get_symbol_intern_id(k), val);
}



void cedar::def_global(runes k, bound_function f) {
  int id = get_symbol_intern_id(k);
  ref func = new lambda(f);
  def_global(id, func);
}



ref cedar::get_global(int id) {
  std::unique_lock<std::mutex> lock(g_lock);

  try {
    return globals.at(id);
  } catch (std::exception & e) {
    throw cedar::make_exception("Unable to find global variable ", get_symbol_id_runes(id));
  }

  return nullptr;
}

ref cedar::get_global(ref k) {
  return get_global(k.as<symbol>()->id);
}

ref cedar::get_global(runes k) {
  return get_global(get_symbol_intern_id(k));
}
