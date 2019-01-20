#include <cedar.h>
#define GC_THREADS
#include <gc.h>
#include <gc/gc_cpp.h>
#define alloc_fn GC_MALLOC
#define free_fn GC_FREE


void* operator new(size_t size) {
	return alloc_fn(size);
}
void* operator new[](size_t size) {
	return alloc_fn(size);
}


#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void operator delete(void* ptr) _NOEXCEPT {
	free_fn(ptr);
}
void operator delete[](void* ptr) _NOEXCEPT {
	free_fn(ptr);
}
