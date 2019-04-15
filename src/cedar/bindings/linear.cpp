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

#include <cedar/modules.h>
#include <cedar/object.h>
#include <cedar/object/module.h>
#include <cedar/object/lambda.h>
#include <cedar/object/string.h>
#include <cedar/native_interface.h>
#include <cedar/objtype.h>
#include <mutex>

using namespace cedar;


type *matrix_type;


class matrix : public object {
 protected:
  int _m, _n;
  double *buf;
  std::mutex _lock;

 public:
  inline matrix() { m_type = matrix_type; }

  inline matrix(int m, int n) {
    resize(m, n);
    m_type = matrix_type;
  }


  inline void resize(int m, int n) {
    _m = m;
    _n = n;
    delete buf;
    buf = new double[m * n];
    for (int i = 0; i < m * n; i++) buf[i] = 0.0;
  }


  inline int m(void) { return _m; }
  inline int n(void) { return _n; }


  inline double get(int m, int n) {
    std::unique_lock lock(_lock);
    if (m > _m || m < 0) throw std::out_of_range("m out of range");
    if (n > _n || n < 0) throw std::out_of_range("n out of range");
    return buf[m * _n + n];
  }

  inline static matrix *column(int m) { return new matrix(m, 1); }

};


void matrix_alloc(const function_callback & args) {
  args.get_return() = new matrix();
};



// matrix new is overloaded.
// (matrix 2 3) -> m = 2, n = 3
// (matrix 20) -> m = 20, n = 1
// (matrix [1 2 3]) -> column vector
// (matrix [[1 2] [3 4]]) -> 2x2 matrix
void matrix_new(const function_callback & args) {
  matrix *self = ref_cast<matrix>(args.self());
  if (args.len() == 2) {

    // here, the arguments must all be ints.
    for (int i = 0; i < 2; i++) {
      if (args[i].get_type() != number_type) {
        printf("opes\n");
        args.throw_obj(new string("matrix constructor requires 2 integers"));
        return;
      }
    }

    int m = args[0].to_int();
    int n = args[1].to_int();
    self->resize(m, n);
    printf("here\n");
    return;
  }
  self->resize(4, 4);

}



void matrix_str(const function_callback & args) {
  matrix *self = ref_cast<matrix>(args.self());
  cedar::runes s;

  s += "(matrix [";
  for (int m = 0; m < self->m(); m++) {
    if (m != 0) s += "         ";
    s += "[";
    for (int n = 0; n < self->n(); n++) {
      s += std::to_string(self->get(m, n));
      if (n < self->n()-1) s += " ";
    }
    s += "]";
    if (m < self->m()-1) s += "\n";
  }
  s += "])";

  args.get_return() = new string(s);
}




void bind_linear(void) {
  module *mod = new module("linear");

  matrix_type = new type("Matrix");


  // matrix_type->setattr(new symbol("__alloc__"), new lambda(matrix_alloc));
  // matrix_type->set_field("new", matrix_new);
  // matrix_type->set_field("str", matrix_str);
  // matrix_type->set_field("repr", matrix_str);

  mod->def("matrix", matrix_type);


  define_builtin_module("linear", mod);
}
