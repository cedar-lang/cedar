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



#include <cedar/object.h>
#include <cedar/vm/instruction.h>
#include <cedar/vm/bytecode.h>
#include <cedar/opcode.h>
#include <vector>

#include <cstdio>
#include <cstdlib>

using namespace cedar;
using namespace cedar::vm;


static std::string instruction_name(instruction &i);

bool instruction::encode(bytecode *bc) {

	// write the opcode first,
	bc->write<uint8_t>(op);

	// then write the arguments
	switch (type()) {

		case imm_object:
			bc->write<object*>(arg_object);
			break;

		case imm_float:
			bc->write<double>(arg_float);
			break;

		case imm_int:
			bc->write<int64_t>(arg_int);
			break;

		case imm_ptr:
			bc->write<void*>(arg_voidptr);
			break;

		case no_arg:
			break;
	}
	return true;
}


inst_type instruction::type(void) {
	switch (op) {

#define OP_CASE(_, code, op_type) case code: return op_type; break;
		CEDAR_FOREACH_OPCODE(OP_CASE);
#undef OP_CASE
		default:
			return no_arg;
	}
	return no_arg;
}



std::vector<instruction> cedar::vm::decode_bytecode(bytecode* bc) {
	uint64_t i = 0;
	std::vector<instruction> insts;

#define READ_INTO(field, type) \
	{ type val = bc->read<type>(i); i += sizeof(val); it.field = val; }; break;

	while (i < bc->get_size()) {
		instruction it;
		it.address = i;
		it.op = bc->read<uint8_t>(i++);
		switch (it.type()) {
			case imm_object: READ_INTO(arg_object, object*);
			case imm_float: READ_INTO(arg_float, double);
			case imm_int: READ_INTO(arg_int, int64_t);
			case imm_ptr: READ_INTO(arg_voidptr, void*);
			// and no_arg is a nop
			case no_arg: {}; break;
		}

		insts.push_back(it);
	}

#undef READ_INTO

	return insts;
}




std::string cedar::vm::instruction::to_string() {
	std::ostringstream buf;



	char hexbuf[12];
	sprintf(hexbuf, "0x%08llx", address);

	buf << hexbuf << "  ";
	buf << instruction_name(*this);
	buf << "\t";


	switch (type()) {

		case imm_object:
			buf << arg_object;
			break;

		case imm_float:
			buf << arg_float;
			break;

		case imm_int:
			buf << arg_int;
			break;

		case imm_ptr:
			buf << arg_voidptr;
			break;

		case no_arg:
			break;
	}

	return buf.str();
}






static std::string instruction_name(instruction &i) {
	switch (i.op) {

#define OP_NAME(name, code, op_type) case code: return #name;
		CEDAR_FOREACH_OPCODE(OP_NAME);
#undef OP_NAME
		default:
			return "unknown";
	}
	return "unknown";
}
