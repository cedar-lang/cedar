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

#include <mutex>

#define GC_THREADS
#include <gc/gc.h>

extern "C" void GC_allow_register_threads();

struct gc_startup {
  gc_startup() {
    GC_set_all_interior_pointers(1);
    GC_INIT();
    GC_allow_register_threads();
  }
};

static gc_startup init;

#define USE_GC

#ifdef USE_GC
#define allocate GC_MALLOC
#define deallocate GC_FREE
#else
#define allocate malloc
#define deallocate free
#endif


#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

// the memory allocator uses an internally linked list to represent the heaps
// with blocks that store the size and flags of a block of memory
struct block {
  int size;

  bool active: 1;
  bool marked: 1;
  block *next;
};

// an arena represents a block of memory used by the garbage collector.
// It is typically a page or more. Allocating an arena will actually allocate
// that region with mmap 
struct arena {
  size_t size;
  arena *next = nullptr;
  char *memory = nullptr;
  std::mutex lock;
};


// allocate a new arena with a minimum size. This is so that if someone asks for
// 4 gb of heap space, we supply it using mmap
arena *allocate_arena(size_t min_size) {
  // the default arena size is 4 pages of memory (16kb)
  static size_t default_stack_size = 4096 * 4;

  size_t size = std::min(ROUND_UP(min_size, 4096), default_stack_size);

  if (size > 4096) printf("%zu %zu\n", min_size, size);

  return nullptr;
}

void *xmalloc(size_t size) {
  return malloc(size);
}


void xfree(void *ptr) {
  // nop
}


void* operator new(size_t size) {
  return allocate(size);
}
void* operator new[](size_t size) {
  return allocate(size);
}


#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void operator delete(void* ptr)_NOEXCEPT {
  deallocate(ptr);
}

void operator delete[](void* ptr) _NOEXCEPT {
  deallocate(ptr);
}


void operator delete(void* ptr, std::size_t s)_NOEXCEPT {
  deallocate(ptr);
}

void operator delete[](void* ptr, std::size_t s) _NOEXCEPT {
  deallocate(ptr);
}
