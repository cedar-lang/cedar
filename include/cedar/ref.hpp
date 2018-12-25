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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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

#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <sstream>
#include <iostream>

#include <cedar/runes.h>
#include <cedar/exception.hpp>


namespace cedar {
	class object;

	// these functions are an ugly workaround for
	// some shortcomings of c++'s circular reference
	// handling. They are defined in src/ref.cc
	uint16_t change_refcount(object *, int);
	uint16_t get_refcount(object *);
	void delete_object(object *);

	object *get_nil_object(void);

	class ref {
		protected:
			object *obj = nullptr;

			inline uint16_t inc() {
				return change_refcount(obj, 1);
			}

			inline uint16_t dec() {
				return change_refcount(obj, -1);
			}

			inline uint16_t rc() {
				return get_refcount(obj);
			}

			inline void release(void) {
				if (obj != nullptr) {
					change_refcount(obj, -1);
					if (get_refcount(obj) == 0) {
						delete_object(obj);
					}
					obj = nullptr;
				}
			}

			inline void retain() {
				if (obj != nullptr) {
					inc();
				} else {
					throw cedar::make_exception("ref attempted to retain access on nullptr");
				}
			}
		public:

			inline ~ref() {
				release();
			}

			inline ref() {
				obj = nullptr;
			};

			inline ref(object *o) {
				set(o);
			}

			inline ref(const ref& other) {
				set(other.obj);
			}

			inline ref& operator=(const ref& other) {
				set(other.obj);
				return *this;
			}

			inline ref& operator=(object *o) {
				set(o);
				return *this;
			}

			inline void set(object *new_ref) {
				release();
				obj = new_ref;
				if (obj != nullptr)
					retain();
			}

			inline object *operator*() const {
				if (obj == nullptr)
					return get_nil_object();
				return obj;
			}

			inline object *operator->() const {
				return operator*();
			}


			ref get_first(void) const;
			ref get_rest(void) const;

			void set_first(ref);
			void set_rest(ref);

			void set_const(bool);


			template<typename T>
				inline T * const get() const {
					return reinterpret_cast<T*>(obj);
				}

			// check to see if the two refs point to the same object
			inline bool operator==(const ref& other) const {
				return other.obj == obj;
			}

			inline bool operator==(const object *other) const {
				return other == obj;
			}

	};

	template<typename T, typename ...Args>
		ref new_obj(Args...args) {
			ref r;
			r.set(new T(args...));
			return r;
		}

	template<typename T, typename ...Args>
		ref new_const_obj(Args...args) {
			ref r = new_obj<T>(args...);
			r.set_const(true);
			return r;
		}
} // namespace cedar
