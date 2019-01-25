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
#include <cedar/fractional.h>
#include <bitset>
#include <cxxabi.h>
#include <cedar/types.h>

namespace cedar {
	class object;


	size_t number_hash_code(void);

	const std::type_info &get_number_typeid(void);
	const std::type_info &get_object_typeid(object*);


	size_t get_hash_code(object *);
	const char *get_object_type_name(object *);


	object *get_nil_object(void);
	u64 get_object_hash(object*);



#define FLAG_FLT 2
#define FLAG_INT 3
#define FLAG_OBJ 4
#define FLAG_PTR 5

	enum ref_type {
		ref_obj,
		ref_int,
		ref_flt,
		ref_ptr, // unknown ref, just a voidptr
	};

	class ref {
		protected:

			std::bitset<8> flags;
			union {
				i64     m_int;
				f64     m_flt;
				object *m_obj;
				void   *m_ptr;

        fractional m_frac;

			};

			void release(void);
			void retain();

		public:


			inline ~ref() {
				release();
			}

			inline ref() {
				clear_type_flags();
				flags[FLAG_OBJ] = true;
				m_obj = nullptr;
			}

			inline ref(object *o) {
				clear_type_flags();
				store_obj(o);
			}


			inline ref(double d) {
				clear_type_flags();
				flags[FLAG_FLT] = true;
				m_flt = d;
			}

			inline ref(i64 i) {
				clear_type_flags();
				flags[FLAG_INT] = true;
				m_int = i;
			}

			inline ref(int i) {
				clear_type_flags();
				flags[FLAG_INT] = true;
				m_int = i;
			}

			inline ref_type rtype(void) {
				if (is_obj()) return ref_obj;
				if (is_flt()) return ref_flt;
				if (is_int()) return ref_int;
				if (is_ptr()) return ref_ptr;
				return ref_obj;
			};


			inline bool is_number(void) const {
				return is_flt() || is_int();
			}
			inline bool is_flt(void) const {
				return flags[FLAG_FLT];
			}
			inline bool is_int(void) const {
				return flags[FLAG_INT];
			}
			inline bool is_obj(void) const {
				return flags[FLAG_OBJ];
			}
			inline bool is_ptr(void) const {
				return flags[FLAG_PTR];
			}


			// clear all the type flags. Useful when changing
			// the value stored inside the ref
			inline void clear_type_flags(void) {
				flags[FLAG_OBJ] = 0;
				flags[FLAG_FLT] = 0;
				flags[FLAG_INT] = 0;
				flags[FLAG_PTR] = 0;
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
				clear_type_flags();
				flags[FLAG_PTR] = true;
				m_ptr = ptr;
				return *this;
			}

			inline ref& operator=(const ref& other) {

				if (is_obj()) release();

				if (other.is_obj()) {
					store_obj(other.m_obj);
				} else {
					// otherwise, copy the whole block of memory
					m_int = other.m_int;
				}
				// copy over the flags
				flags = other.flags;
				return *this;
			}

			inline ref& operator=(ref&& other) {

				if (is_obj()) release();

				if (other.is_obj()) {
					store_obj(other.m_obj);
				} else {
					// otherwise, copy the whole block of memory
					m_int = other.m_int;
				}
				// copy over the flags
				flags = other.flags;
				return *this;
			}

			inline ref& operator=(object *o) {
				store_obj(o);
				return *this;
			}

			inline void store_obj(object *a_obj) {
				release();
				m_obj = a_obj;
				clear_type_flags();
				flags[FLAG_OBJ] = true;
				retain();
			}

			inline object *operator*() const {
				return m_obj == nullptr ? get_nil_object() : m_obj;
			}
			inline object *operator->() const {
				return operator*();
			}



			template<typename T>
				inline T num_cast(void) {
					if (is_flt()) {
						return (T)m_flt;
					}
					if (is_int()) {
						return (T)m_int;
					}
					throw cedar::make_exception("attempt to cast non-number reference to a number");
				}

			inline double to_float(void) {
				return num_cast<double>();
			}
			inline i64 to_int(void) {
				return num_cast<i64>();
			}


			ref first(void) const;
			ref rest(void) const;
      ref cons(ref);

			void set_first(ref);
			void set_rest(ref);

			void set_const(bool);

			uint64_t symbol_hash(void);


			template<typename T>
				inline T * get() const {
					return is_obj() ? dynamic_cast<T*>(m_obj) : nullptr;
				}

			template<typename T>
				inline T reinterpret(void) const {
					return reinterpret_cast<T>(m_obj);
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
			 */
			template<typename T>
				inline T *as() const {
					return is_obj() ? dynamic_cast<T*>(m_obj) : nullptr;
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
				return get_object_type_name(m_obj);
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
				} else if (is_obj()) {
					given_name = get_object_typeid(m_obj).name();
				} else given_name = "void";
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
				if (is_flt()) {
					return *reinterpret_cast<u64*>(&m_flt);
				}
				if (is_int()) {
					return m_int;
				}
				if (is_nil()) return 0lu;
				auto h = get_object_hash(m_obj);
				return h;
			}


#define FOREACH_OP(V)\
			V(add, +)      \
			V(sub, -)      \
			V(mul, *)      \
			V(div, /)

			enum binop {
				#define V(name, op) name,
					FOREACH_OP(V)
				#undef V
			};

			static inline ref binary_op(binop op, ref & a, ref & b) {
				if (a.is_number() && b.is_number()) {
					switch (op) {
						#define V(name, op) \
							case name: {\
													 return (a.is_flt() || b.is_flt()) ? ref((a.to_float() op b.to_float())) : ref((i64)(a.to_int() op b.to_int())); \
												 }

							FOREACH_OP(V)
						#undef V
						default:
							throw cedar::make_exception("unknown op");
					}
				}

				switch (op) {
					#define V(name, op) case name: throw cedar::make_exception("Attempt to " #name " two non-numbers: ", a.to_string(), " " #op " ", b.to_string());
					FOREACH_OP(V)
					#undef V
				}
			}

#undef FOREACH_OP




			////////////////////////////////////////////////////////////////////////
			inline ref operator+(ref other) {
				return binary_op(add, *this, other);
			}

			inline ref operator-(ref other) {
				return binary_op(sub, *this, other);
			}

			inline ref operator*(ref other) {
				return binary_op(mul, *this, other);
			}

			inline ref operator/(ref other) {
				return binary_op(div, *this, other);
			}

			inline int compare(ref& other) const {
				ref self = const_cast<ref&>(*this);
				if (!is_number() || !other.is_number()) {
					return self.hash() - other.hash();
				}
				return binary_op(sub, self, other).to_int();
			}


			inline bool operator<(ref other) const {
				return compare(other) < 0;
			}
			inline bool operator<=(ref other) const {
				return compare(other) <= 0;
			}
			inline bool operator>(ref other) const {
				return compare(other) > 0;
			}
			inline bool operator>=(ref other) const {
				return compare(other) >= 0;
			}
			inline bool operator==(ref other) const {
				if (!is_number())
					return (typeid(m_obj).hash_code() == typeid(other.m_obj).hash_code()) && other.hash() == const_cast<ref&>(*this).hash();
				return compare(other) == 0;
			}

			inline bool operator!=(ref other) {
				return !operator==(other);
			}

			////////////////////////////////////////////////////////////////////////


	};

	inline std::ostream& operator<<(std::ostream& os, ref o) {
		os << o.to_string();
		return os;
	}

	template<typename T, typename ...Args>
		ref new_obj(Args...args) {
			ref r = new T(args...);
			return r;
		}

	template<class T>
		constexpr inline T *ref_cast(const ref &r) {
			return r.as<T>();
		}



} // namespace cedar

template <>
struct std::hash<cedar::ref> {
	std::size_t operator()(const cedar::ref& k) const {
		return const_cast<cedar::ref&>(k).hash();
	}
};
