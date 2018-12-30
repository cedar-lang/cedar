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
#include <bitset>
#include <cxxabi.h>

namespace cedar {
	class object;

	// these functions are an ugly workaround for
	// some shortcomings of c++'s circular reference
	// handling. They are defined in src/ref.cc
	uint16_t change_refcount(object *, int);
	uint16_t get_refcount(object *);
	void delete_object(object *);


	size_t number_hash_code(void);

	const std::type_info &get_number_typeid(void);
	const std::type_info &get_object_typeid(object*);


	size_t get_hash_code(object *);


	object *get_nil_object(void);





#define FLAG_NUMBER 0
#define FLAG_FLOAT  1


	class ref {
		protected:

			std::bitset<8> flags;
			union {

				union {
					double m_float;
					int64_t m_int;
				};

				object *obj = nullptr;
			};


			inline uint16_t inc() {
				if (!is_object()) return 0;
				return change_refcount(obj, 1);
			}

			inline uint16_t dec() {
				if (!is_object()) return 0;
				return change_refcount(obj, -1);
			}

			inline uint16_t rc() {
				if (!is_object()) return 0;
				return get_refcount(obj);
			}

			inline void release(void) {
				if (is_object() && obj != nullptr) {
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


			inline double to_float(void) {
				if (is_int()) {
					return m_int;
				}
				return m_float;
			}


			inline int64_t to_int(void) {
				if (is_int()) {
					return m_int;
				}
				return m_float;
			}


		public:

			inline ~ref() {
				release();
			}

			inline ref() {
				obj = nullptr;
			};

			inline ref(object *o) {
				flags[FLAG_NUMBER] = false;
				flags[FLAG_FLOAT]  = false;
				set(o);
			}


			inline ref(double d) {
				flags[FLAG_NUMBER] = true;
				flags[FLAG_FLOAT]  = true;
				m_float = d;
			}

			inline ref(int64_t i) {
				flags[FLAG_NUMBER] = true;
				flags[FLAG_FLOAT] = false;
				m_int = i;
			}

			inline ref(int i) {
				flags[FLAG_NUMBER] = true;
				flags[FLAG_FLOAT] = false;
				m_int = i;
			}



			inline bool is_object(void) const {
				return !flags[FLAG_NUMBER];
			}
			inline bool is_number(void) const {
				return flags[FLAG_NUMBER];
			}

			inline bool is_float(void) const {
				return flags[FLAG_FLOAT];
			}
			inline bool is_int(void) const {
				return !flags[FLAG_FLOAT];
			}

			inline ref(const ref& other) {
				flags = other.flags;
				set(other.obj);
			}

			inline ref& operator=(const ref& other) {
				flags = other.flags;
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
				inline T * get() const {
					return reinterpret_cast<T*>(obj);
				}

			/*
			 * is<T>
			 *
			 * returns if the object is a T or not.
			 */
			template<typename T>
				inline bool is() const {
					return typeid(T).hash_code() == type_id();
				}

			/*
			 * as<T>
			 *
			 * attempts to cast the object to a shared pointer of another object type
			 */
			template<typename T>
				inline T *as() {
					return dynamic_cast<T*>(obj);
				}

			/*
			 * type_id
			 *
			 * returns the type hash_code from typeid()
			 */
			inline size_t type_id() const {
				if (is_number()) {
					return get_number_typeid().hash_code();
				}
				return get_object_typeid(obj).hash_code();
			}



			/*
			 * type_name
			 *
			 * returns the c++ abi demangled type name of the polymorphic object class
			 * that is extending the cedar::object class
			 */
			inline cedar::runes type_name() {

				int status;

				const char *given_name;

				if (is_number()) {
					given_name = get_number_typeid().name();
				} else {
					given_name = get_object_typeid(obj).name();
				}

				// use the c++ abi to demangle the name that typeid gives back;
				char *realname = abi::__cxa_demangle(given_name, 0, 0, &status);
				if (status) {
					throw cedar::make_exception("unable to demangle name of type with the c++ abi: ", given_name);
				}

				cedar::runes r = realname;
				delete realname;
				return r;
			}


			cedar::runes to_string(bool human = false);


			////////////////////////////////////////////////////////////////////////
			inline ref operator+(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to add non-numbers: ", to_string(), " + ", other.to_string());
				}
				if (is_float() || other.is_float()) {
					return to_float() + other.to_float();
				}
				return to_int() + other.to_int();
			}

			inline ref operator-(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to subtract non-numbers: ", to_string(), " - ", other.to_string());
				}
				if (is_float() || other.is_float()) {
					return to_float() - other.to_float();
				}
				return to_int() - other.to_int();
			}

			inline ref operator*(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to multiply non-numbers: ", to_string(), " * ", other.to_string());
				}
				if (is_float() || other.is_float()) {
					return to_float() * other.to_float();
				}
				return to_int() * other.to_int();
			}

			inline ref operator/(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to divide non-numbers: ", to_string(), " / ", other.to_string());
				}
				if (is_float() || other.is_float()) {
					return to_float() / other.to_float();
				}
				return to_int() / other.to_int();
			}


			inline int compare(ref& other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to compare non-numbers: ", to_string(), " and ", other.to_string());
				}
				return operator-(other).to_int();
			}


			inline bool operator<(ref other) {
				return compare(other) < 0;
			}
			inline bool operator<=(ref other) {
				return compare(other) <= 0;
			}
			inline bool operator>(ref other) {
				return compare(other) > 0;
			}
			inline bool operator>=(ref&other) {
				return compare(other) >= 0;
			}
			inline bool operator==(ref other) {
				if (!is_number()) return other.obj == obj;
				return compare(other) == 0;
			}

#define LITERAL_BINARY_OP_TYPE(op, type) \
			inline ref operator op(const type& val) { \
				if (is_float()) return to_float() + val;  \
				return to_int() + val;                    \
			}

// unsed literal binary op macro
#define LITERAL_BINARY_OP(op) LITERAL_BINARY_OP_TYPE(op, double); LITERAL_BINARY_OP_TYPE(op,int);

			////////////////////////////////////////////////////////////////////////


	};

	inline std::ostream& operator<<(std::ostream& os, ref o) {
		os << o.to_string();
		return os;
	}

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
