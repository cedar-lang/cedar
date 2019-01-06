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

#include <cstdint>

#include <cedar/vm/bytecode.h>

namespace cedar {

	class object;


	namespace vm {


		enum inst_type {
			no_arg,
			imm_object,
			imm_float,
			imm_int,
			imm_ptr,
		};

		// an instruction is an internal representation of
		// a single bytecode instruction in the VM.
		//
		// TODO: determine if the VM should use the instruction
		//       type only durring the optimization step and use
		//       a true byte based instruction format durring
		//       execution or use this instruction format as the
		//       real execution thread instruction
		class instruction {
			public:

				uint64_t address = 0;

				// an opcode between 0 and 255. Used in the VM to lookup
				// where to jump to in the jump table for threaded exec
				uint8_t op;

				// the following anonymous union allows an instruction to
				// store various types of information in the instruciton
				// itself
				union {
					// constant object argument
					object *arg_object;
					// floating point arguments
					double arg_float;
					// basic integer argument types
					int64_t arg_int;
					void *arg_voidptr;
				};

				bool encode(bytecode&);

				inst_type type(void);
				std::string to_string();
		};



		std::vector<instruction> decode_bytecode(bytecode*);

	} // namespace vm
} // namespace cedar
