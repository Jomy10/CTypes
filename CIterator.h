#ifndef _CTYPES_ITERATOR_H
#define _CTYPES_ITERATOR_H

#include "CArray.h"
#include <stddef.h>

// TODO
#ifndef __nonnull
#define __nonnull
#endif
#ifndef __nullable
#define __nullable
#endif

typedef int(*CmpFn)(const void* a, const void* b);

enum IteratorOptionSet {
  ITER_CONTIGUOUS = 0x0001,
  ITER_KNOWNSIZE = 0x0010,
  ITER_ENUMERATED = 0x0100,
};

typedef struct Iterator {
  enum IteratorOptionSet opt;
  void* __nullable data;
  // iterdata_t* __nonnull data;
  void*(* __nonnull next)(void*);
  /// available if `ITER_ENUMERATED`
  long* idx;
  /// available if `ITER_KNOWNSIZE`
  size_t known_size;
  /// The size of an element returned by this iterator
  size_t type_size;
  /// available if `ITER_CONTIGUOUS`
  void* __nullable contiguous_buffer;
  /// Optional free
  void(* __nullable free)(void*);
} iter_t;

typedef struct EnumeratedValue {
  long i;
  void* inner_data;
  void(* __nullable inner_free)(void*);
  void*(* __nonnull inner_next)(void*);
} enumeratedValue_t;

// typedef struct EnumeratedIteratorData {
//   enumeratedValue_t value;
//   void* __nullable data;
//   void*(* __nonnull next)(void*);
// } enumeratedIteratorData_t;

typedef struct ArrayIterData {
  array_t* storage;
  long idx;
} arrayiter_t;

#ifdef __cplusplus
extern "C" {
#endif

iter_t* array_createIterator(array_t* arr);

void iter_destroy(iter_t* iter);

void* iter_next(iter_t* iter);
array_t* iter_collect(iter_t* iter, array_t* outArr);
array_t* iter_collectCreate(iter_t* iter);

/// Returns `outArr`
/// The array will be reset before inserting new values
array_t* iter_map(iter_t* iter, array_t* outArr, void(*mutate)(const void* in, void* out));
/// Maps an iterator into a newly created array which has to be destroyed
/// Functions post-fixed with `create` return an array that should be destroyed by the user
array_t* iter_mapCreate(iter_t* iter, void(*mutate)(const void* in, void* out));

/// Reduces an iterator into `intoValue`
/// Returns `intoValue`
const void* iter_reduce(iter_t* iter, void* intoValue, void(*reduce)(const void* in, void* out));

bool iter_allSatisfy(iter_t* iter, bool(*where)(const void*));

/// `intoArray` should have the same type_size as `arr`
/// New values will be pushed to the array
array_t* iter_findAll(iter_t* iter, array_t* intoArray, bool(*where)(const void*));
/// The returned array should be destroyed by the user
array_t* iter_findAllCreate(iter_t* iter, bool(*where)(const void*));

/// `intoArray` should be an array of type `size_t`
array_t* iter_findAllIndexes(iter_t* iter, array_t* intoArray, bool(*where)(const void*));
array_t* iter_findAllIndexesCreate(iter_t* iter, bool(*where)(const void*));

const void* iter_findFirst(iter_t* iter, bool(*where)(const void*));
/// Find the last occurence where the `where` condition is fullfilled
// const void* iter_findLast(iter_t* iter, bool(*where)(const void*));

long long iter_indexOfFirst(iter_t* iter, bool(*where)(const void*));
// long long iter_indexOfLast(iter_t* iter, bool(*where)(const void*));

/// Values will be pushed to the array
// array_t* iter_sorted(iter_t* iter, array_t* outArr, CmpFn compare/*, algorithm function*/);
// array_t* iter_sortedCreate(iter_t* iter, CmpFn compare/*, algorithm function*/);

/// Returns NULL if the iterator cannot be reversed
// iter_t* iter_reversed(iter_t* iter);

/// Return the maximum value in an iterator
const void* iter_max(iter_t* iter, CmpFn compare);
const void* iter_min(iter_t* iter, CmpFn compare);

/// `iter` will be invalidated
iter_t* iter_enumerated(iter_t* iter);

#ifdef CT_ITERATOR_IMPL

#include <string.h>
#include <stdlib.h>

void* _arrayiter_next(void* data) {
  arrayiter_t* iter = (arrayiter_t*)data;
  return array_getChecked(iter->storage, ++iter->idx);
}

iter_t* array_createIterator(array_t* arr) {
  iter_t* iter = malloc(sizeof(iter_t) + sizeof(arrayiter_t));
  if (iter == NULL) return NULL;
  arrayiter_t* arriter = ((void*)iter) + sizeof(iter_t);

  arriter->storage = arr;
  arriter->idx = -1;

  iter->opt = ITER_KNOWNSIZE | ITER_CONTIGUOUS | ITER_ENUMERATED;
  iter->data = arriter;
  iter->next = _arrayiter_next;
  iter->type_size = arr->type_size;

  iter->known_size = arr->size;
  iter->idx = &arriter->idx;
  iter->contiguous_buffer = arr->data;

  iter->free = free;

  return iter;
}

void iter_destroy(iter_t* iter) {
  if (iter->free != NULL)
    iter->free(iter);
}

void* iter_next(iter_t* iter) {
  return iter->next(iter->data);
}

array_t* iter_collect(iter_t* iter, array_t* outArr) {
  if (iter->opt & ITER_KNOWNSIZE)
    array_reserveAtLeast(outArr, iter->known_size);

  if ((iter->opt & ITER_CONTIGUOUS) && (iter->opt & ITER_KNOWNSIZE)) {
    memcpy(outArr->data, iter->contiguous_buffer, iter->known_size * iter->type_size);
    outArr->size = iter->known_size;
    return outArr;
  }

  void* data;
  while ((data = iter_next(iter))) {
    array_push(outArr, data);
  }

  return outArr;
}

array_t* iter_collectCreate(iter_t* iter) {
  array_t* arr;
  if (iter->opt & ITER_KNOWNSIZE) {
    arr = array_createWithCap(iter->type_size, iter->known_size);
  } else {
    arr = array_create(iter->type_size);
  };
  return iter_collect(iter, arr);
}

array_t* iter_map(iter_t* iter, array_t* outArr, void(*mutate)(const void* in, void* out)) {
  iter_t* it = iter;
  if ((iter->opt & ITER_ENUMERATED) == 0) {
    it = iter_enumerated(iter);
  }

  const void* value;
  if (it->opt & ITER_KNOWNSIZE) {
    if (it->known_size <= outArr->size) {
      if (array_reserveAtLeast(outArr, it->known_size)) { return NULL; }
      outArr->size = it->known_size;
    }
    while ((value = iter_next(it))) {
      mutate(value, array_get(outArr, *(it->idx)));
    }
  } else {
    outArr->size = 0;
    size_t cap = 10;
    array_reserveAtLeast(outArr, cap);
    while ((value = iter_next(it))) {
    // while ((value = iter_next(it))) {
      if (*(it->idx) == cap) {
        cap *= 2;
        array_grow(outArr, cap);
      }
      mutate(value, array_get(outArr, *(it->idx)));
    }
    outArr->size = *(it->idx);
  }
  return outArr;
}

array_t* iter_mapCreate(iter_t* iter, void(*mutate)(const void* in, void* out)) {
  array_t* arr = array_create(iter->type_size);
  iter_map(iter, arr, mutate);
  return arr;
}

const void* iter_reduce(iter_t* iter, void* intoValue, void(*reduce)(const void* in, void* out)) {
  const void* value;
  while ((value = iter_next(iter))) {
    reduce(value, intoValue);
  }
  return value;
}

bool iter_allSatisfy(iter_t* iter, bool(*where)(const void*)) {
  const void* value;
  while ((value = iter_next(iter))) {
    if (!where(value)) return false;
  }
  return true;
}

array_t* iter_findAll(iter_t* iter, array_t* intoArray, bool(*where)(const void*)) {
  void* value;
  while ((value = iter_next(iter))) {
    if (where(value))
      array_push(intoArray, value);
  }
  return intoArray;
}

array_t* iter_findAllCreate(iter_t* iter, bool(*where)(const void*)) {
  array_t* arr = array_create(iter->type_size);
  return iter_findAll(iter, arr, where);
}

array_t* iter_findAllIndexes(iter_t* iter, array_t* intoArray, bool(*where)(const void*)) {
  iter_t* it = iter;
  if ((iter->opt & ITER_ENUMERATED) == 0)
    it = iter_enumerated(iter);

  const void* value;
  while ((value = iter_next(it))) {
    if (where(value))
      array_push(intoArray, (it->idx));
  }
  return intoArray;
}

array_t* iter_findAllIndexesCreate(iter_t* iter, bool(*where)(const void*)) {
  array_t* arr = array_create(sizeof(size_t));
  return iter_findAllIndexes(iter, arr, where);
}

const void* iter_findFirst(iter_t* iter, bool(*where)(const void*)) {
  const void* value;
  while ((value = iter_next(iter))) {
    if (where(value)) return value;
  }
  return NULL;
}

long long iter_indexOfFirst(iter_t* iter, bool(*where)(const void*)) {
  iter_t* it = iter;
  if ((iter->opt & ITER_ENUMERATED) == 0)
    it = iter_enumerated(iter);

  const void* value;
  while ((value = iter_next(it))) {
    if (where(value)) return *(it->idx);
  }
  return -1;
}

const void* iter_max(iter_t* iter, CmpFn compare) {
  void* currentMax = iter_next(iter);
  void* value;
  while ((value = iter_next(iter))) {
    if (compare(currentMax, value) > 0) {
      currentMax = value;
    }
  }
  return currentMax;
}

const void* iter_min(iter_t* iter, CmpFn compare) {
  void* currentMin = iter_next(iter);
  void* value;
  while ((value = iter_next(iter))) {
    if (compare(currentMin, value) < 0) {
      currentMin = value;
    }
  }
  return currentMin;
}

const void* iter_min(iter_t* iter, CmpFn compare);

void* _iter_enumerated_next(void* data) {
  enumeratedValue_t* val = (enumeratedValue_t*)data;
  val->i += 1;
  return val->inner_next(val->inner_data);
}

void _iter_enumerated_free(void* data) {
  enumeratedValue_t* val = (enumeratedValue_t*)data;
  if (val->inner_free)
    val->inner_free(val->inner_data);
  free(val);
}

iter_t* iter_enumerated(iter_t* iter) {
  if (iter->opt & ITER_ENUMERATED) return iter;
  iter->opt |= ITER_ENUMERATED;
  enumeratedValue_t* val = malloc(sizeof(enumeratedValue_t));
  val->i = -1;
  val->inner_data = iter->data;
  val->inner_free = iter->free;
  val->inner_next = iter->next;

  iter->data = (void*)val;
  iter->idx = &val->i;
  iter->free = _iter_enumerated_free;
  iter->next = _iter_enumerated_next;

  return iter;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
