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

#include <cedar/exception.hpp>

#include <cstdint>
#include <map>
#include <list>

namespace cedar {

	class object;


	namespace vm {

		// bytecode represents the actual bytecode that goes into
		// encoding instructions in the VM. It is a true "byte" code
		// where multi-byte values need to be intereted out of a
		// stream of bytes
		//
		// functions for reading and writing arbitrary values to
		// the byte stream are included behind templates
		//
		// typically a VM will only have one instance of bytecode
		//
		// bytecode also contains a map of symbol addresses inside
		// the bytecode, and a list of addresses that need linked
		// to these symbols for function binding
		class bytecode {

			private:
				// a mapping of unresolved references to other parts of
				// the bytecode. This is populated with static jumps and
				// when a lambda is defined in-language
				//
				// Whenever the compiler registers a label with the bytecode,
				// it will look through the list of unresolved references for
				// jumps and function refs that require that label's bytecode
				// address
				std::map<std::string, std::list<uint64_t*>> unresolved_references;



				// the code pointer is where this instance's bytecode is actually
				// stored. All calls for read interpret a type from this byte vector
				// and calls for write append to it.
				uint8_t *code;

				// size is how many bytes are written into the code pointer
				// it also determines *where* to write when writing new data
				uint64_t size = 0;
				// cap is the number of bytes allocated for the code pointer
				uint64_t cap = 255;

			public:

				inline bytecode() {
					code = new uint8_t[255];
				}


				template<typename T>
					inline T read(uint64_t i) const {
						if (sizeof(T) + i >= cap)
							throw cedar::make_exception("bytecode unable to read ", sizeof(T), " bytes from index ", i, ". Out of bounds.");
						return *(T*)(void*)(code+i);
					}


				template<typename T>
					inline uint64_t write_to(uint64_t i, T val) const {
						if (sizeof(T) + i >= cap)
							throw cedar::make_exception("bytecode unable to write ", sizeof(T), " bytes to index ", i, ". Out of bounds.");
						*(T*)(void*)(code+i) = val;
						return size;
					}

				template<typename T>
					inline uint64_t write(T val) {
						if (size + sizeof(val) >= cap-1) {
							uint8_t *new_code = new uint8_t[cap * 2];
							memcpy(new_code, code, cap);
							cap *= 2;
							delete code;
							code = new_code;
						}
						uint64_t addr = write_to(size, val);
						size += sizeof(T);
						return addr;
					}

				inline uint64_t get_size() {
					return size;
				}
				inline uint64_t get_cap() {
					return cap;
				}
		};

	} // namespace vm
} // namespace cedar
