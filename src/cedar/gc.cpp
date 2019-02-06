#include <cedar.h>
#include <stdio.h>
#define GC_THREADS
#include <gc/gc.h>

struct gc_startup {
  gc_startup() {
    GC_INIT();
  }
};

static gc_startup init;



void* operator new(size_t size) {
  void *obj = GC_MALLOC(size);
	return obj;
}
void* operator new[](size_t size) {
  void *obj = GC_MALLOC(size);
	return obj;
}


#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void operator delete(void* ptr) _NOEXCEPT {
	// GC_FREE(ptr);
}
void operator delete[](void* ptr) _NOEXCEPT {
	// GC_FREE(ptr);
}


void operator delete(void* ptr, std::size_t) _NOEXCEPT {
	// GC_FREE(ptr);
}
void operator delete[](void* ptr, std::size_t) _NOEXCEPT {
	// GC_FREE(ptr);
}
