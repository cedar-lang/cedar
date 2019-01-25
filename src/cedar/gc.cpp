#include <cedar.h>
#include <stdio.h>
#define GC_THREADS
#include <gc/gc.h>

struct gc_startup {
  gc_startup() {
    GC_INIT();
    // GC_enable_incremental();
    // GC_allow_register_threads();
  }
};

static gc_startup init;


/*



void obj_finalizer(void *obj, void *x) {
  printf("%p finalized\n", obj);
}

void* operator new(size_t size) {
  void *obj = GC_MALLOC(size);
  GC_register_finalizer(obj, obj_finalizer, NULL, 0, 0);
	return obj;
}
void* operator new[](size_t size) {
  void *obj = GC_MALLOC(size);
  GC_register_finalizer(obj, obj_finalizer, NULL, 0, 0);
	return obj;
}


#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void operator delete(void* ptr) _NOEXCEPT {
	GC_FREE(ptr);
}
void operator delete[](void* ptr) _NOEXCEPT {
	GC_FREE(ptr);
}
*/
