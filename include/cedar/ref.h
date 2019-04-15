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

#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>

#include <cedar/fractional.h>
#include <cedar/runes.h>
#include <cedar/types.h>
#include <cxxabi.h>
#include <bitset>
#include <cedar/exception.hpp>
#include <mutex>

namespace cedar {

  class type;
  class object;

  size_t number_hash_code(void);

  const std::type_info &get_number_typeid(void);
  const std::type_info &get_object_typeid(object *);

  size_t get_hash_code(object *);
  const char *get_object_type_name(object *);

  object *get_nil_object(void);
  u64 get_object_hash(object *);


#define BIT_READ(x,y) ((0u == (x & (1l<<y)))?0u:1u)
#define BIT_SET(number, bit) ((number) |= (1l<<(bit)))
#define BIT_CLR(number, bit) ((number) &= (~(1l<<(bit))))



  class ref {
   public:


#ifdef COMPRESSED_REF
#define FLAG_FLOAT 63
#define FLAG_INT 62
    union {
      object *m_obj;
      int64_t m_flags;
#pragma pack(push, 1)
      struct {
        union {
          int32_t m_int;
          float m_flt;
        };
        int32_t __buffer;
      };
#pragma pack(pop)
    };
#else

#define FLAG_FLOAT 7
#define FLAG_INT 6

    union {
      object *m_obj;
      int64_t m_int;
      double m_flt;
    };
    u8 m_flags;

#endif
     /*
    */

   public:
    ref();
    ref(object *o);
    ref(double d);
    ref(i64 i);
    ref(int i);


    inline bool is_number(void) const { return is_flt() || is_int(); }
    inline bool is_flt(void) const { return BIT_READ(m_flags, FLAG_FLOAT); }
    inline bool is_int(void) const { return BIT_READ(m_flags, FLAG_INT); }
    inline bool is_obj(void) const {
      return !is_flt() && !is_int();
    }

    // clear all the type flags. Useful when changing
    // the value stored inside the ref
    inline void clear_type_flags(void) {
      m_flags = 0;
    }

    bool is_nil(void) const;

    inline ref(const ref &other) { operator=(other); }
    inline ref(ref &&other) { operator=(other); }


    inline object *get(void) { return is_obj() ? m_obj : nullptr; }

    inline ref &operator=(const ref &other) {
      m_obj = other.m_obj;
      m_flags = other.m_flags;
      return *this;
    }

    inline ref &operator=(ref &&other) {
      m_obj = other.m_obj;
      *this = other;
      return *this;
    }

    inline ref &operator=(object *o) {
      m_obj = o;
      m_flags = 0;
      return *this;
    }

    inline void store_obj(object *a_obj) {
      m_obj = a_obj;
      m_flags = 0;
    }

    inline object *operator*() const {
      return m_obj == nullptr ? get_nil_object() : m_obj;
    }
    inline object *operator->() const { return operator*(); }

    template <typename T>
    inline T num_cast(void) {
      if (is_flt()) {
        return (T)m_flt;
      }
      if (is_int()) {
        return (T)m_int;
      }
      throw cedar::make_exception(
          "attempt to cast non-number reference to a number");
    }

    inline double to_float(void) { return num_cast<double>(); }
    inline i64 to_int(void) { return num_cast<i64>(); }

    ref first(void) const;
    ref rest(void) const;
    ref cons(ref);

    void set_first(ref);
    void set_rest(ref);

    void set_const(bool);

    uint64_t symbol_hash(void);

    template <typename T>
    inline T *get() const {
      return is_obj() ? dynamic_cast<T *>(m_obj) : nullptr;
    }

    template <typename T>
    inline T reinterpret(void) const {
      return reinterpret_cast<T>(m_obj);
    }

    template <typename T>
    inline T stat_cast(void) const {
      return static_cast<T>(m_obj);
    }

    /*
     * is<T>
     *
     * returns if the object is a T or not.
     */
    template <typename T>
    inline bool is() const {
      return typeid(T).hash_code() == type_id();
    }

    /*
     * as<T>
     */
    template <typename T>
    inline T *as() const {
      if (is_obj()) {
        return static_cast<T *>(m_obj);
      }
      return nullptr;
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

    type *get_type(void) const;
    ref getattr(ref);
    void setattr(ref, ref);

    bool isa(type *) const;

    bool is_seq(void);

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
      } else
        given_name = "void";
      // use the c++ abi to demangle the name that typeid gives back;
      char *realname = abi::__cxa_demangle(given_name, 0, 0, &status);
      if (status) {
        throw cedar::make_exception(
            "unable to demangle name of type with the c++ abi: ", given_name);
      }

      cedar::runes r = realname;
      delete realname;
      return r;
    }

    cedar::runes to_string(bool human = false);

    inline u64 hash(void) {
      if (is_flt()) {
        return *reinterpret_cast<u64 *>(&m_flt);
      }
      if (is_int()) {
        return m_int;
      }
      if (is_nil()) return 0lu;
      auto h = get_object_hash(m_obj);
      return h;
    }

#define FOREACH_OP(V) \
  V(add, +)           \
  V(sub, -)           \
  V(mul, *)           \
  V(div, /)

    enum binop {
#define V(name, op) name,
      FOREACH_OP(V)
#undef V
    };

    ref binary_op(binop op, ref &a, ref &b);

    ////////////////////////////////////////////////////////////////////////
    inline ref operator+(ref other) { return binary_op(add, *this, other); }

    inline ref operator-(ref other) { return binary_op(sub, *this, other); }

    inline ref operator*(ref other) { return binary_op(mul, *this, other); }

    inline ref operator/(ref other) { return binary_op(div, *this, other); }

    inline int compare(ref &other) {
      ref self = const_cast<ref &>(*this);
      if (!is_number() || !other.is_number()) {
        return self.hash() - other.hash();
      }

      if (other.get_type() != get_type()) {
        return -1;
      }
      return binary_op(sub, self, other).to_int();
    }

    inline bool operator<(ref other) { return compare(other) < 0; }
    inline bool operator<=(ref other) { return compare(other) <= 0; }
    inline bool operator>(ref other) { return compare(other) > 0; }
    inline bool operator>=(ref other) { return compare(other) >= 0; }
    bool operator==(ref other);

    inline bool operator!=(ref other) { return !operator==(other); }

    ////////////////////////////////////////////////////////////////////////
  };

  inline std::ostream &operator<<(std::ostream &os, ref o) {
    os << o.to_string();
    return os;
  }

  template <typename T, typename... Args>
  ref new_obj(Args... args) {
    ref r = new T(args...);
    return r;
  }

  template <class T>
  constexpr inline T *ref_cast(const ref &r) {
    return r.as<T>();
  }

}  // namespace cedar

template <>
struct std::hash<cedar::ref> {
  std::size_t operator()(const cedar::ref &k) const {
    return const_cast<cedar::ref &>(k).hash();
  }
};
