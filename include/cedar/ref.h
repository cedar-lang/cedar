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
#include <cedar/types.h>

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
	const char *get_object_type_name(object *);


	object *get_nil_object(void);
	u64 get_object_hash(object*);


#define FLAG_NUMBER 0
#define FLAG_DISABLE_REFCOUNT 1


	enum ref_type {
		ref_obj,
		ref_int,
		ref_flt,
	};

	class ref {
		protected:

			std::bitset<8> flags;
			union {
				i64 m_int;
				f64 m_flt;
				double m_number;
				object *obj = nullptr;
			};


			inline uint16_t inc() {
				if (!flags[FLAG_DISABLE_REFCOUNT])
					return change_refcount(obj, 1);
				return 0;
			}

			inline uint16_t dec() {
				if (!flags[FLAG_DISABLE_REFCOUNT])
					return change_refcount(obj, -1);
				return 0;
			}

			inline uint16_t rc() {
				if (!flags[FLAG_DISABLE_REFCOUNT])
					return get_refcount(obj);
				return 0;
			}

			inline void release(void) {
				if (flags[FLAG_DISABLE_REFCOUNT]) return;
				if (is_object() && obj != nullptr) {
					if (dec() == 0) {
						delete_object(obj);
					}
					obj = nullptr;
				}
			}

			inline void retain() {
				if (flags[FLAG_DISABLE_REFCOUNT]) return;
				if (is_object() && obj != nullptr) {
					inc();
				}
			}

		public:

			inline ~ref() {
				release();
			}

			inline ref() {
				obj = nullptr;
				flags[FLAG_NUMBER] = false;
			}

			inline ref(object *o) {
				flags[FLAG_NUMBER] = false;
				set(o);
			}


			inline ref(double d) {
				flags[FLAG_NUMBER] = true;
				m_number = d;
			}

			inline ref(int64_t i) {
				flags[FLAG_NUMBER] = true;
				m_number = i;
			}

			inline ref(long int i) {
				flags[FLAG_NUMBER] = true;
				m_number = i;
			}

			inline ref(int i) {
				flags[FLAG_NUMBER] = true;
				m_number = i;
			}

			inline ref_type rtype(void) {
				return ref_obj;
			};



			inline bool is_object(void) const {
				return flags[FLAG_NUMBER] == false;
			}


			inline bool is_number(void) const {
				return flags[FLAG_NUMBER] == true;
			}


			inline bool is_flt(void) const {
				return false; // flags[FLAG_FLT];
			}

			bool is_nil(void) const;


			inline ref(const ref& other) {
				operator=(other);
			}
			inline ref(ref&& other) {
				operator=(other);
			}

			// for storing arbitrary pointers. This behavior is for low-level
			// VM operations and should ignore all refcounting after assigning this
			inline ref & store_ptr(void * ptr) {
				flags[FLAG_DISABLE_REFCOUNT] = true;
				obj = (object*)ptr;
				return *this;
			}

			inline ref& operator=(const ref& other) {
				flags[FLAG_DISABLE_REFCOUNT] = other.flags[FLAG_DISABLE_REFCOUNT];
				if (other.is_number()) {
					m_number = other.m_number;
				} else if (other.is_object()) {
					set(other.obj);
				}
				flags = other.flags;
				return *this;
			}
			inline ref& operator=(ref&& other) {
				flags[FLAG_DISABLE_REFCOUNT] = other.flags[FLAG_DISABLE_REFCOUNT];
				if (other.is_number()) {
					m_number = other.m_number;
				} else if (other.is_object()) {
					set(other.obj);
				}
				flags = other.flags;
				return *this;
			}

			inline ref& operator=(object *o) {
				set(o);
				return *this;
			}

			inline void set(object *new_ref) {
				release();
				obj = new_ref;
				flags[FLAG_NUMBER] = false;
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


			inline double to_float(void) {
				if (!flags[FLAG_NUMBER])
					throw cedar::make_exception("attempt to cast non-number reference to a number");
				return m_number;
			}


			ref get_first(void) const;
			ref get_rest(void) const;

			void set_first(ref);
			void set_rest(ref);

			void set_const(bool);

			uint64_t symbol_hash(void);


			template<typename T>
				inline T * get() const {
					return dynamic_cast<T*>(obj);
				}

			template<typename T>
				inline T reinterpret(void) const {
					return reinterpret_cast<T>(obj);
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
				inline T *as() const {
					return is_object() ? dynamic_cast<T*>(obj) : nullptr;
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
				return get_object_typeid(operator*()).hash_code();
			}


			inline const char *object_type_name(void) {
				if (is_number()) return "number";
				if (is_nil()) return "nil";
				return get_object_type_name(obj);
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

				printf(":: %s\n", given_name);

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


			inline u64 hash(void) {
				if (is_number()) {
					return *reinterpret_cast<u64*>(&m_number);
				}
				if (is_nil()) return 0lu;
				auto h = get_object_hash(obj);
				return h;
			}


			////////////////////////////////////////////////////////////////////////
			inline ref operator+(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to add non-numbers: ", to_string(), " + ", other.to_string());
				}
				return to_float() + other.to_float();
			}

			inline ref operator-(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to subtract non-numbers: ", to_string(), " - ", other.to_string());
				}
				return to_float() - other.to_float();
			}

			inline ref operator*(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to multiply non-numbers: ", to_string(), " * ", other.to_string());
				}
				return to_float() * other.to_float();
			}

			inline ref operator/(ref other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to divide non-numbers: ", to_string(), " / ", other.to_string());
				}
				return to_float() / other.to_float();
			}


			inline int compare(ref& other) {
				if (!is_number() || !other.is_number()) {
					throw cedar::make_exception("attempt to compare non-numbers: ", to_string(), " and ", other.to_string());
				}
				return operator-(other).to_float();
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
				if (!is_number())
					return (typeid(obj).hash_code() == typeid(other.obj).hash_code()) && other.hash() == hash();
				return compare(other) == 0;
			}

			inline bool operator!=(ref other) {
				return !operator==(other);
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

	template<class T>
		constexpr inline T *ref_cast(const ref &r) {
			return r.as<T>();
		}

} // namespace cedar
