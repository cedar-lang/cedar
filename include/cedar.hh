#ifndef CEDAR_HH
#define CEDAR_HH

#include <memory>

namespace cedar {
	// supply an alias to a shorter form of std::shared_ptr<T>
	// that can be used by either cedar::ptr<T> or ptr<T> internally
	template<typename T>
		using ptr = std::shared_ptr<T>;

} // namespace cedar

#endif
