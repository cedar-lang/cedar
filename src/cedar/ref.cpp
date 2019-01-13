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

#include <cedar/object.h>
#include <cedar/object/list.h>
#include <cedar/object/number.h>
#include <cedar/object/sequence.h>
#include <cedar/object/symbol.h>
#include <cedar/object/nil.h>
#include <cedar/ref.h>

using namespace cedar;


void cedar::ref::retain(void) {
	if (is_obj() && m_obj != nullptr) {
		m_obj->refcount++;
	}
}

void cedar::ref::release(void) {
	if (is_obj() && m_obj != nullptr) {
		m_obj->refcount--;
		if (m_obj->refcount == 0) {
			delete m_obj;
		}
		m_obj = nullptr;
	}
}

ref cedar::ref::get_first() const {
  if (!is_obj())
    throw cedar::make_exception("unable to get first of non-object reference");
  if (m_obj == nullptr) return nullptr;
  return reinterpret_cast<sequence *>(m_obj)->get_first();
}

ref cedar::ref::get_rest() const {
  if (!is_obj())
    throw cedar::make_exception("unable to get rest of non-object reference");
  if (m_obj == nullptr) return nullptr;
  return reinterpret_cast<sequence *>(m_obj)->get_rest();
}

void cedar::ref::set_first(ref val) {
  if (!is_obj())
    throw cedar::make_exception("unable to set first of non-object reference");
  if (m_obj == nullptr) return;
  return reinterpret_cast<sequence *>(m_obj)->set_first(val);
}

void cedar::ref::set_rest(ref val) {
  if (!is_obj())
    throw cedar::make_exception("unable to set rest of non-object reference");
  if (m_obj == nullptr) return;
  return reinterpret_cast<sequence *>(m_obj)->set_rest(val);
}

cedar::runes ref::to_string(bool human) {

	if (is_int()) {
		return std::to_string(m_int);
	}
  if (is_number()) {
    auto str = std::to_string(m_flt);
    long len = str.length();
    for (int i = len - 1; i > 0; i--) {
      if (str[i] == '0') {
				str.pop_back();
      } else if (str[i] == '.') {
				str.pop_back();
				break;
      } else {
				break;
      }
    }
    return str;
  }
  if (m_obj == nullptr) return U"nil";
  return m_obj->to_string(human);
}

const std::type_info &cedar::get_number_typeid(void) {
  return typeid(cedar::number);
}
const std::type_info &cedar::get_object_typeid(object *obj) {
  return typeid(*obj);
}

// returns the hash of the symbol, otherwise returns 0
uint64_t cedar::ref::symbol_hash(void) {
  if (is_obj())
    if (auto sym = ref_cast<cedar::symbol>(*this); sym != nullptr) {
      return sym->hash();
    }
  return 0ull;
}

bool cedar::ref::is_nil(void) const {
  if (is_obj()) {
    if (m_obj == nullptr) return true;
		if (is<cedar::nil>()) return true;
		/*
    if (auto *list = ref_cast<cedar::list>(*this); list != nullptr) {
    	if (list->get_first().is_nil() && list->get_rest().is_nil()) return true;
    }
		*/
  }
  return false;
}

const char *cedar::get_object_type_name(object *obj) {
  return obj->object_type_name();
}

u64 cedar::get_object_hash(object *obj) {
	return obj->hash();
}

