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


#include <cedar/ref.h>
#include <cedar/vm/bytecode.h>
#include <cedar/vm/instruction.h>
#include <cedar/vm/opcode.h>
#include <unistd.h>
#include <functional>

using namespace cedar;


ref &vm::bytecode::get_const(int index) { return constants[index]; }

int vm::bytecode::push_const(ref val) {
  int index = constants.size();
  constants.push_back(val);
  return index;
}


static void each_opcode(int size, uint8_t *code, std::function<void(uint8_t)> func) {
#define WALK_OVER(type)   \
  { i += sizeof(type); }; \
  break;
  int i = 0;
  while (i < size) {
    vm::instruction it;
    it.address = i;
    func(code[i++]);
    switch (it.type()) {
      case vm::imm_object:
        WALK_OVER(object *);
      case vm::imm_float:
        WALK_OVER(double);
      case vm::imm_int:
        WALK_OVER(int64_t);
      case vm::imm_ptr:
        WALK_OVER(void *);
      case vm::no_arg: {
      }; break;
    }
  }

#undef WALK_OVER
}


void vm::bytecode::finalize(void) {
  int stack_effect = 0;
  each_opcode(size, code, [&stack_effect](uint8_t op) {
#define V(name, code, type, effect) \
  case code:                        \
    stack_effect += effect;         \
    break;
    switch (op) { CEDAR_FOREACH_OPCODE(V) }
#undef V
  });


  stack_size = stack_effect;
}



void vm::bytecode::print(u8 *ip) {
  auto ins = decode_bytecode(this);
  int path_width = 5;
  for (auto i : ins) {
    path_width += i.op == OP_JUMP || i.op == OP_JUMP_IF_FALSE;
  }
  path_width *= 2;
  std::vector<std::string> paths(ins.size());
  for (auto &s : paths) {
    for (int i = 0; i < path_width + 8; i++) {
      s += ' ';
    }
  }
  int icount = ins.size();
  int path_i = 0;
  for (int i = 0; i < icount; i++) {
    auto c = ins[i];
    bool is_jump = c.op == OP_JUMP || c.op == OP_JUMP_IF_FALSE;
    if (is_jump) {
      path_i++;
      int target_index = 0;
      for (int t = 0; t < icount; t++) {
        if (ins[t].address == (u64)c.arg_int) target_index = t;
      }
      paths[i][0] = '>';
      paths[target_index][0] = '<';
      for (int p = 0; p < path_i * 2; p++) {
        paths[i][p + 1] = '-';
        paths[target_index][p + 1] = '-';
      }
      int start = std::min(i, target_index);
      int end = std::max(i, target_index);
      paths[start][path_i * 2] = '.';
      paths[end][path_i * 2] = '\'';
      for (int p = start + 1; p < end; p++) {
        paths[p][path_i * 2] = '|';
      }
    }
  }
  for (int i = 0; i < icount; i++) {
    std::string is = ins[i].to_string(0);
    while (is.size() < 35) is += ' ';

    bool is_current = false;

    ins[i].address += (u64)code;
    if (ins[i].address == (u64)ip) {
      is_current = true;
    }


    if (isatty(STDOUT_FILENO)) {
      if (is_current) {
        printf("\x1b[33m");
      } else {
        printf("\x1b[90m");
      }
    }
    std::cout << is << " " << paths[i] << std::endl;

    if (isatty(STDOUT_FILENO)) {
      if (is_current) printf("\x1b[0m");
    }
  }
}
