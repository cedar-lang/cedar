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


#include <cedar/ref.h>
#include <cedar/vm/bytecode.h>
#include <cstdio>
#include <vector>
#include <stack>
#include <memory>
#include <map>
#include <cedar/types.h>


namespace cedar {
	namespace vm {

		// forwared declaration
		class machine;

		struct compiler_ctx {
      bool inside_catch = false;
			u64 closure_size = 0;
			u16 lambda_depth = 0;
		};

		class scope {
			/**
			 * m_bindings
			 * a mapping from the hashes of symbols to their index
			 * in the closure
			 * Looking up a binding's index is an O(n) task as it needs to
			 * walk up the scope tree until it finds the mapping or has no
			 * parent to continue searching. If no value was found, -1 will
			 * be returned as -1 is not a valid closure index
			 */
			std::map<uint64_t, int> m_bindings;
			public:
				std::shared_ptr<scope> m_parent = nullptr;
				scope(std::shared_ptr<scope>);

				int find(uint64_t);
				int find(ref &);

				void set(ref &, int);
				void set(uint64_t, int);

		};

		using scope_ptr = std::shared_ptr<scope>;

		class compiler {
			public:
				cedar::vm::machine *m_vm;
				compiler(cedar::vm::machine *vm);
				~compiler();


				class context {
        };

				/*
				 * given some object reference,
				 * compile it into bytecode and return
				 * the address of the start of the bytecode
				 * representation of that object
				 */
				ref compile(ref, machine*);

				void compile_lambda_expression(ref, bytecode &, scope_ptr sc, compiler_ctx*);
				void compile_progn(ref, bytecode &, scope_ptr, compiler_ctx*);
				void compile_number(ref, bytecode &, scope_ptr, compiler_ctx*);
				void compile_object(ref, bytecode &, scope_ptr, compiler_ctx*);
				void compile_constant(ref, bytecode &, scope_ptr, compiler_ctx*);
				void compile_symbol(ref, bytecode &, scope_ptr, compiler_ctx*);
				void compile_list(ref, bytecode &, scope_ptr, compiler_ctx*);
        void compile_vector(ref, bytecode &, scope_ptr, compiler_ctx*);
				void compile_call_arguments(ref, bytecode &, scope_ptr sc, compiler_ctx*);
				void compile_quasiquote(ref, bytecode &, scope_ptr sc, compiler_ctx*);
		};

		// a bytecode compilation pass that takes in the compiler
		// oboejct
		ref bytecode_pass(ref, compiler*);
	}
}
