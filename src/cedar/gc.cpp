#include <cedar.h>
#include <stdio.h>

// Include file for GC-aware allocators.
// alloc allocates uncollected objects that are scanned by the collector
// for pointers to collectable objects.  Gc_alloc allocates objects that
// are both collectable and traced.  Single_client_alloc and
// single_client_gc_alloc do the same, but are not thread-safe even
// if the collector is compiled to be thread-safe.  They also try to
// do more of the allocation in-line.
#include <gc/gc_allocator.h>

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
  GC_free(ptr);
}

void operator delete[](void* ptr) _NOEXCEPT {
  GC_free(ptr);
}


void operator delete(void* ptr, std::size_t s) _NOEXCEPT {
  GC_free(ptr);
}

void operator delete[](void* ptr, std::size_t s) _NOEXCEPT {
  GC_free(ptr);
}
