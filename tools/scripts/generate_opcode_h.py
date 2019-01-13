# This script generates the opcode.h header file.

import sys
import tokenize

header = """\
/* Auto-generated by tools/scripts/generate_opcode_h.py */
#pragma once
#ifndef __OPCODE_H
#define __OPCODE_H

/* Instruction opcodes for compiled code */
"""

footer = """
#endif
"""


ops = []

# push a new opcode to the list of opcode
# defaults to having no stack effect
def new_op(name, inst_type='no_arg', effect=0):
    ops.append((name.upper(), inst_type, effect))


new_op('NOP', effect=0)
new_op('NIL', effect=1)

# load the constant at the index
new_op('CONST', 'imm_int', effect=1)

# push a float literal to the stack
new_op('FLOAT', 'imm_float', effect=1)


new_op('INT',   'imm_int', effect=1)
new_op('LOAD_LOCAL', 'imm_int', effect=1) # load a local from the start of the stack frame
new_op('SET_LOCAL', 'imm_int', effect=0) # set a local from the start of the stack frame

# pop the name off the stack, look it up, then push the value found,
# otherwise throw because it wasn't found
#
# These instructions are used only to access global bindings not found in local scope or
# freevars
new_op('LOAD_GLOBAL', 'imm_int', effect=0)
# SET_GLOBAL pops the name off the stack, then the value off the stack
#  GLOBALS[POP()] = POP(); PUSH(GLOBALS[...]);
new_op('SET_GLOBAL', 'imm_int', effect=0)

new_op('CONS', effect=-1);

new_op('CALL', effect=-1);
new_op('MAKE_FUNC', 'imm_int', effect=1);

# arg_pop pops one value from the argument list and updates the list
# at the argument index in the stack call frame
new_op('ARG_POP', 'imm_int', effect=1);

new_op('RETURN', effect=0);
new_op('EXIT', effect=0);

new_op('SKIP', effect=-1);

new_op('MAKE_CLOSURE', effect=0)

def main(outfile):
    with open(outfile, 'w') as f:
        f.write(header)

        for i, op in enumerate(ops):
            f.write("#define OP_%-24s 0x%02x\n" % (op[0], i))

        f.write("\n")
        f.write("/* Instruction opcode foreach macro for code generation */\n")
        f.write("/* Arg order: (name, bytecode, type, stack effect */\n")
        f.write("#define CEDAR_FOREACH_OPCODE(V) \\\n")
        for i, op in enumerate(ops):
            f.write("  V(%s, OP_%s, %s, %s)" % (op[0], op[0], op[1], op[2]))
            if i < len(ops)-1:
                f.write(" \\")
            f.write("\n")
        f.write(footer)


if __name__ == '__main__':
    main('include/cedar/vm/opcode.h')















