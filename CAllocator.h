#ifndef _CTYPES_ALLOCATOR_H
#define _CTYPES_ALLOCATOR_H

#include <stddef.h>

typedef struct Allocator {
  void*(*alloc)(size_t, void*);
  void*(*realloc)(size_t, void*);
  void(*free)(void*, void*);
} allocator_t;

#endif
