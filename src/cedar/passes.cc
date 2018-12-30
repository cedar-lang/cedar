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

#include <cedar/passes.h>
#include <cedar/ref.hpp>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/object/nil.h>


using namespace cedar;

cedar::passcontroller::passcontroller(ref val) {
	m_val = val;
}


ref passcontroller::get(void) {
	return m_val;
}


#define PASS_DEBUG
passcontroller& passcontroller::pipe(pass_function f) {

	ref oldval = m_val;
	m_val = f(m_val);
	m_pass_count++;
#ifdef PASS_DEBUG
	std::cout << "PASS " << m_pass_count << ": \n   from: " << oldval << "\n     to: " << m_val << std::endl;
#endif
	return *this;
}


ref cedar::wrap_top_level_with_lambdas(ref val) {



	std::vector<ref> expr;
	expr.push_back(cedar::new_obj<symbol>("lambda"));
	expr.push_back(cedar::new_obj<list>());
	expr.push_back(val);
	ref lambda = cedar::new_obj<list>(expr);

	return lambda;
}
