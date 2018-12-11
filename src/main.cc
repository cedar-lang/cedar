// (c) 2018 Nick Wanninger
// This code is licensed under MIT license (see LICENSE for details)

#include <cstdio>
#include <iostream>
#include <cedar/util.hpp>
#include <cedar/parser.hh>

using namespace cedar;


int main(int argc, const char **argv) {

	std::locale utf8(std::locale(), new std::codecvt_utf8_utf16<wchar_t>);
	std::wcout.imbue(utf8);

	std::wstring src;
	std::cout << argc;
	if (argc > 1) {
		src = util::read_file(argv[1]);
	}


	try {
		auto r = std::make_shared<reader>(src);
		r->run();


	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
		exit(-1);
	}

	return 0;
}
