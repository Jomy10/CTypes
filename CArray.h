#ifndef _CTYPE_ARRAY_H
#define _CTYPE_ARRAY_H

// #include "CAllocator.h"
#include <stddef.h>
#include <stdbool.h>

#ifndef CARRAY_DEFAULT_CAP
#define CARRAY_DEFAULT_CAP 10
#endif

typedef int(*ArrayCmpFn)(const void* a, const void* b);
typedef void(*ArraySortFn)(void* base, size_t num, size_t size, ArrayCmpFn compare);

typedef struct Array {
  size_t size;
  size_t cap;
  /// The size of the type stored in this array
  size_t type_size;
  void* data;
} array_t;

#define Array(T) array_t*

#ifdef __cplusplus
extern "C" {
#endif

// Public //

// == Create ==
array_t* array_create(size_t type_size);
// array_t* array_createWithAllocator(size_t type_size, allocator_t* a);
array_t* array_createWithCap(size_t type_size, size_t cap);
// array_t* array_createWithCapAndAllocator(size_t type_size, size_t cap, allocator_t* a);

// == Destroy ==
void array_destroy(array_t*);

// == Single value methods ==
bool array_hasIndex(const array_t* arr, size_t idx);

void* array_get(const array_t* arr, size_t idx);
/// Returns `NULL` if the index doesn't exist
void* array_getChecked(const array_t* arr, size_t idx);

/// Returns NULL if size is 0
void* array_first(const array_t* arr);
/// Returns NULL if size is 0
void* array_last(const array_t* arr);

/// Copies the data of `value` to the array
void array_set(array_t* arr, size_t idx, const void* value);
/// Returns 1 if the index doesn't exist
int array_setChecked(array_t* arr, size_t idx, const void* value);

/// Returns 1 if memory could not be allocated
int array_insert(array_t* arr, size_t idx, const void* value);
/// Returns 1 if memory could not be allocated
int array_push(array_t* arr, const void* value);
/// Returns 1 if memory could not be allocated
int array_pushFirst(array_t* arr, const void* value);
/// Remove the last element and store its value in `outData` (if `outData` is not NULL)
/// Returns the new size or -1 if size is already 0
/// The type of `outData` should be the type the array was initialized with
long array_pop(array_t* arr, void* outData);
/// Remove the element at `idx` and store its value in `outData` (if `outData` is not NULL)
/// Returns the new size or -1 if size is already 0
/// The type of `outData` should be the type the array was initialized with
long array_popAt(array_t* arr, size_t idx, void* outData);
/// Remove the first element and store its value in `outData` (if `outData` is not NULL)
/// Returns the new size or -1 if size is already 0
/// The type of `outData` should be the type the array was initialized with
long array_popFirst(array_t* arr, void* outData);

// == Memory ==

/// Increases the capacit of the array to the given capacity
/// No check is performed on `newCap` > `arr->newCap`
/// Returns 1 if memory couldn't be allocated
int array_grow(array_t* arr, size_t newCap);

/// Returns 1 if memory couldn't be allocated
int array_reserveAtLeast(array_t* arr, size_t newCap);

void array_reset(array_t* arr);
void array_resetRemovingCapacity(array_t* arr);

void array_swap(array_t* arr, size_t idx1, size_t idx2);

/// Sorts the array in place.
/// Returns `arr`
array_t* array_sort(array_t* arr, ArrayCmpFn compare, ArraySortFn sort);

/// Reverses the array in place
/// Returns `arr`
array_t* array_reverse(array_t* arr);

#ifdef CT_ARRAY_IMPL

#include <stdlib.h>
#include <string.h>

int _array_initializeMemory(array_t* arr, size_t count) {
  arr->cap = count;
  arr->data = malloc(arr->cap * arr->type_size);
  return arr->data == NULL;
}

int _array_growIfNecessary(array_t* arr) {
  if (arr->cap == arr->size) {
    if (arr->cap == 0) {
      _array_initializeMemory(arr, CARRAY_DEFAULT_CAP);
      if (arr->data == NULL) return 1;
    } else {
      return array_grow(arr, arr->cap * 2);
    }
  }
  return 0;
}

array_t* array_create(size_t type_size) {
  array_t* arr = calloc(1, sizeof(array_t));
  arr->type_size = type_size;
  return arr;
}

array_t* array_createWithCap(size_t type_size, size_t cap) {
  array_t* arr = array_create(type_size);
  _array_initializeMemory(arr, cap);
  return arr;
}

void array_destroy(array_t* arr) {
  free(arr->data);
  free(arr);
}

bool array_hasIndex(const array_t* arr, size_t idx) {
  return arr->size > idx;
}

void* array_get(const array_t* arr, size_t idx) {
  return (arr->data + arr->type_size * idx);
}

void* array_getChecked(const array_t* arr, size_t idx) {
  if (!array_hasIndex(arr, idx)) return NULL;
  return array_get(arr, idx);
}

void* array_first(const array_t* arr) {
  if (arr->size == 0) return NULL;
  return array_get(arr, 0);
}

void* array_last(const array_t* arr) {
  if (arr->size == 0) return NULL;
  return array_get(arr, arr->size - 1);
}

void array_set(array_t* arr, size_t idx, const void* value) {
  memcpy(array_get(arr, idx), value, arr->type_size);
}

int array_setChecked(array_t* arr, size_t idx, const void* value) {
  if (!array_hasIndex(arr, idx)) return 1;
  array_set(arr, idx, value);
  return 0;
}

int array_insert(array_t* arr, size_t idx, const void* value) {
  if (_array_growIfNecessary(arr) != 0) return 1;
  memmove(array_get(arr, idx + 1), array_get(arr, idx), (arr->size - idx) * arr->type_size);
  memcpy(arr->data + idx * arr->type_size, value, arr->type_size);
  arr->size += 1;
  return 0;
}

int array_push(array_t* arr, const void* value) {
  if (_array_growIfNecessary(arr) != 0) return 1;
  memcpy(array_get(arr, arr->size), value, arr->type_size);
  arr->size += 1;
  return 0;
}

int array_pushFirst(array_t* arr, const void* value) {
  return array_insert(arr, 0, value);
}

long array_pop(array_t* arr, void* outData) {
  if (arr->size == 0) return -1;
  arr->size -= 1;
  if (outData != NULL)
    memcpy(outData, array_get(arr, arr->size), arr->type_size);
  return arr->size;
}

long array_popAt(array_t* arr, size_t idx, void* outData) {
  if (arr->size == 0) return -1;
  arr->size -= 1;
  if (outData != NULL)
    memcpy(outData, array_get(arr, idx), arr->type_size);
  memmove(array_get(arr, idx), array_get(arr, idx + 1), arr->type_size * (arr->size - idx));
  return arr->size;
}

long array_popFirst(array_t* arr, void* outData) {
  return array_popAt(arr, 0, outData);
}

int array_grow(array_t* arr, size_t newCap) {
  arr->cap = newCap;
  arr->data = realloc(arr->data, newCap * arr->type_size);
  if (arr->data == NULL) return 1;
  return 0;
}

int array_reserveAtLeast(array_t* arr, size_t newCap) {
  if (arr->cap >= newCap) return 0;
  return array_grow(arr, newCap);
}

void array_reset(array_t* arr) {
  arr->size = 0;
}

void array_resetRemovingCapacity(array_t* arr) {
  array_reset(arr);
  arr->cap = 0;
  free(arr->data);
}

void array_swap(array_t* arr, size_t idx1, size_t idx2) {
  unsigned char tmp[arr->type_size];
  void* a = array_get(arr, idx1);
  void* b = array_get(arr, idx2);
  memcpy(tmp, a, arr->type_size);
  memmove(a, b, arr->type_size);
  memcpy(b, tmp, arr->type_size);
}

array_t* array_sort(array_t* arr, ArrayCmpFn cmp, ArraySortFn sort) {
  sort(arr->data, arr->size, arr->type_size, cmp);
  return arr;
}

array_t* array_reverse(array_t* arr) {
  for (size_t i = 0; i < arr->size / 2; i++) {
    array_swap(arr, i, arr->size - i - 1);
  }
  return arr;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
