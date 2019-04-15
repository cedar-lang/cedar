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

#include <cedar/serialize.h>

#include <cedar/objtype.h>
#include <cedar/object/vector.h>
#include <cedar/object/dict.h>
#include <cedar/object/lambda.h>
#include <cedar/object/symbol.h>
#include <cedar/object/list.h>
#include <cedar/object/keyword.h>
#include <cedar/object/string.h>



using namespace cedar;

serializer::serializer(FILE *_fp) {
  fp = _fp;
}





/**
 * i -> int
 * f -> float
 * s -> string
 * r -> symbol
 * k -> keyword
 * c -> cons cell (list node)
 * n -> nil
 * v -> vector
 * l -> lambda
 * d -> dict
 */


void serializer::write(ref obj) {

  type *otype = obj.get_type();

  if (otype == number_type) {
    if (obj.is_int()) {
      i64 val = obj.to_int();
      fprintf(fp, "i");
      fwrite(&val, sizeof(i64), 1, fp);
      return;
    }
    if (obj.is_flt()) {
      auto val = obj.to_float();
      fprintf(fp, "f");
      fwrite(&val, sizeof(double), 1, fp);
      return;
    }
    return;
  }


  if (otype == string_type) {
    std::string s = obj.to_string(true);
    fprintf(fp, "s");
    int len = s.size();
    fwrite(&len, sizeof(len), 1, fp);
    fprintf(fp, "%s", s.c_str());
    return;
  }

  if (otype == symbol_type) {
    std::string s = obj.to_string(true);
    fprintf(fp, "r");
    int len = s.size();
    fwrite(&len, sizeof(len), 1, fp);
    fprintf(fp, "%s", s.c_str());
    return;
  }


  if (otype == keyword_type) {
    std::string s = obj.to_string(true);
    fprintf(fp, "k");
    int len = s.size();
    fwrite(&len, sizeof(len), 1, fp);
    fprintf(fp, "%s", s.c_str());
    return;
  }


  if (otype == list_type) {
    fprintf(fp, "c");
    write(obj.first());
    write(obj.rest());
    return;
  }


  if (otype == nil_type) {
    fprintf(fp, "n");
    return;
  }


  if (otype == vector_type) {
    fprintf(fp, "v");
    vector *v = ref_cast<vector>(obj);
    int len = v->size();
    fwrite(&len, sizeof(int), 1, fp);
    for (int i = 0; i < len; i++) {
      write(v->get(i));
    }
    return;
  }


  if (otype == lambda_type) {
    lambda *l = ref_cast<lambda>(obj);

    if (l->code_type == lambda::bytecode_type) {
      fprintf(fp, "l");
      // first print the metadata about the function
      // print the name
      write(l->name);
      // write the defining structure
      write(l->defining);

      // write the arg index
      fwrite(&l->arg_index, sizeof(l->arg_index), 1, fp);
      // write the argc
      fwrite(&l->argc, sizeof(l->argc), 1, fp);
      // write the vararg status
      fwrite(&l->vararg, sizeof(l->vararg), 1, fp);


      // encode the bytecode itself
      vm::bytecode *code = l->code;
      // print the constant length
      int const_size = code->constants.size();
      fwrite(&const_size, sizeof(const_size), 1, fp);
      for (int i = 0; i < const_size; i++) {
        write(code->constants[i]);
      }

      i64 size = code->get_size();
      // write the bytecode size
      fwrite(&size, sizeof(code->get_size()), 1, fp);
      // the stack size of the bytecode
      fwrite(&code->stack_size, sizeof(code->stack_size), 1, fp);
      // and print the actual instruction stream
      fwrite(code->code, size, 1, fp);

      return;
    } else {
      throw std::logic_error("unable to serialize non-bytecode function");
    }
  }




  if (otype == dict_type) {
    fprintf(fp, "d");
    dict *d = ref_cast<dict>(obj);
    // print the number of items
    int size = d->size();

    fwrite(&size, sizeof(size), 1, fp);
    // defer to the dict to write
    d->encode(this);
    return;
  }

  throw std::invalid_argument("unable to serialize object");
}



#define READ_INTO(v) fread(&(v), sizeof((v)), 1, fp)

static std::string read_string(FILE *fp, int len) {
  std::string s;

  for (int i = 0; i < len; i++) {
    char c;
    READ_INTO(c);
    s += c;
  }

  return s;
}


ref cedar::serializer::read(void) {
  // read the first char
  char t;
  READ_INTO(t);

  if (t == 'f') {
    double n;
    READ_INTO(n);
    return n;
  }
  if (t == 'i') {
    i64 n;
    READ_INTO(n);
    return n;
  }

  if (t == 'n') {
    return nullptr;
  }

  if (t == 's') {
    int len;
    READ_INTO(len);
    std::string s = read_string(fp, len);
    return new string(s);
  }


  if (t == 'r') {
    int len;
    READ_INTO(len);
    std::string s = read_string(fp, len);
    return new symbol(s);
  }


  if (t == 'k') {
    int len;
    READ_INTO(len);
    std::string s = read_string(fp, len);
    return new keyword(s);
  }

  if (t == 'c') {
    ref first = read();
    ref rest = read();
    return new list(first, rest);
  }


  if (t == 'v') {
    immer::flex_vector<ref> items;
    int len;
    READ_INTO(len);
    for (int i = 0; i < len; i++) {
      items = items.push_back(read());
    }
    return new vector(items);
  }


  if (t == 'd') {
    dict *d = new dict();
    // print the number of items
    int size;
    READ_INTO(size);

    for (int i = 0; i < size; i++) {
      ref k = read();
      ref v = read();
      d->set(k, v);
    }
    return d;
  }


  if (t == 'l') {
    lambda *l = new lambda();
    l->name = read();
    l->defining = read();
    READ_INTO(l->arg_index);
    READ_INTO(l->argc);
    READ_INTO(l->vararg);
    vm::bytecode *code = new vm::bytecode();
    int const_size;
    READ_INTO(const_size);

    for (int i = 0; i < const_size; i++) {
     code->constants.push_back(read());
    }

    READ_INTO(code->size);
    code->cap = code->size;
    code->code = new uint8_t[code->cap];
    READ_INTO(code->stack_size);

    fread(code->code, code->cap, 1, fp);

    l->code = code;
    return l;

  }
  return nullptr;
}
