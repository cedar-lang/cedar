#ifndef CEDAR_HH
#define CEDAR_HH

#include <memory>
#include <string>
#include <cstdint>
#include <utf8.h>
#include <iostream>
#include <cedar/runes.hh>

namespace cedar {
	// supply an alias to a shorter form of std::shared_ptr<T>
	// that can be used by either cedar::ptr<T> or ptr<T> internally
	template<typename T>
		using ptr = std::shared_ptr<T>;

	template<typename T, typename ...Args>
		inline ptr<T> make(Args... args) {
			return std::make_shared<T>(args...);
		}

} // namespace cedar

#endif
