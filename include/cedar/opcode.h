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

#include <cedar/vm/instruction.h>

#define OP_NOP         0x00
#define OP_PUSH_NIL    0x01
#define OP_PUSH_NILS   0x02
#define OP_PUSH_TRUE   0x03
#define OP_LOAD_CONST  0x04




#define OP_PUSH_FLOAT  0x10

#define inst_push_float(val) cedar::vm::instruction{ .op = OP_PUSH_FLOAT, .arg_float = (val) }
#define inst_add() cedar::vm::instruction{ .op = OP_ADD }

// CEDAR_FOREACH_OPCODE takes some macro function V(name, opcode)
#define CEDAR_FOREACH_OPCODE(F) \
	F(NOP,    OP_NOP,         no_arg)  /* push a single nil to the stack */ \
	F(NIL,    OP_PUSH_NIL,    no_arg)  /* push a single nil to the stack */ \
	F(NILS,   OP_PUSH_NILS,   imm_int) /* push n nils to the stack */ \
	F(T,      OP_PUSH_TRUE,   no_arg)  /* loads a nil onto the stack */  \
	F(CONST,  OP_LOAD_CONST,  imm_int) /* loads a function's nth constant onto the stack */ \
	F(PUSH_FLOAT, OP_PUSH_FLOAT, imm_float) /* pushes a single float to the stack */ \
