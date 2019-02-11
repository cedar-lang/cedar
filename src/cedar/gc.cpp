#include <cedar.h>
#include <stdio.h>


#define GC_THREADS
#include <gc/gc.h>


extern "C" void GC_allow_register_threads();

struct gc_startup {
  gc_startup() {
    GC_INIT();

    GC_allow_register_threads();
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
}

void operator delete[](void* ptr) _NOEXCEPT {
}


void operator delete(void* ptr, std::size_t s) _NOEXCEPT {
}

void operator delete[](void* ptr, std::size_t s) _NOEXCEPT {
}
