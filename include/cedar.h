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

#ifndef CEDAR_HH
#define CEDAR_HH

#include "cedar/version.h"
#include "cedar/parser.h"
#include "cedar/context.h"
#include "cedar/vm/bytecode.h"
#include "cedar/vm/compiler.h"
#include "cedar/vm/machine.h"
#include "cedar/vm/instruction.h"
#include "cedar/object_types.h"
#include "cedar/object/symbol.h"
#include "cedar/object/keyword.h"
#include "cedar/object/nil.h"
#include "cedar/object/list.h"
#include "cedar/object/sequence.h"
#include "cedar/object/indexable.h"
#include "cedar/object/number.h"
#include "cedar/object/lambda.h"
#include "cedar/object/string.h"
#include "cedar/object/dict.h"
#include "cedar/object/lazy_sequence.h"
#include "cedar/object/vector.h"
#include "cedar/memory.h"
#include "cedar/object.h"
#include "cedar/runes.h"
#include "cedar/util.hpp"
#include "cedar/vm/opcode.h"
#include "cedar/exception.hpp"
#include "cedar/ref.h"
#include "cedar/vm/binding.h"
#include "cedar/types.h"

#endif
