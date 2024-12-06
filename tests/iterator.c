#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#define CT_ARRAY_IMPL
#include "../CArray.h"
#define CT_ITERATOR_IMPL
#include "../CIterator.h"

#define INTVAL(ptr) (*((int*)ptr))

void* nextValue(void* storage) {
  *((int*)storage) += 1;
  return storage;
}

void addOne(const void* in, void* out) {
  *((int*)out) = INTVAL(in) + 1;
}

void* nextUpTo10(void* storage) {
  *((int*)storage) += 1;
  if (INTVAL(storage) == 10) return NULL;
  return storage;
}

void summing(const void* in, void* out) {
  *((int*)out) += *((int*)in);
}

int intCmp(const void* a, const void* b) {
  return *((int*)b) - *((int*)a);
}

int main(void) {
  // Next
  int iterData = 0;
  iter_t intIter = (iter_t) {
    .opt = 0,
    .data = &iterData,
    .next = nextValue,
    .type_size = sizeof(int),
    .free = NULL
  };

  assert(INTVAL(iter_next(&intIter)) == 1);
  assert(INTVAL(iter_next(&intIter)) == INTVAL(iter_next(&intIter)) - 1);

  iter_destroy(&intIter);

  // Collect
  array_t* arr = array_create(sizeof(int));
  for (int i = 0; i < 10; i++)
    array_push(arr, &i);
  iter_t* arrIter = array_createIterator(arr);

  array_t* into = array_create(sizeof(int));
  iter_collect(arrIter, into);
  assert(arr->size == into->size);
  for (int i = 0; i < arr->size; i++) {
    assert(INTVAL(array_get(arr, i)) == INTVAL(array_get(into, i)));
  }

  iter_destroy(arrIter);
  array_destroy(arr);
  array_destroy(into);

  // Map
  arr = array_create(sizeof(int));
  for (int i = 0; i < 10; i++)
    array_push(arr, &i);
  iter_t* iter = array_createIterator(arr);

  arr = iter_map(iter, arr, addOne);

  for (int i = 0; i < 10; i++)
    assert(INTVAL(array_get(arr, i)) == i + 1);

  array_destroy(arr);
  iter_destroy(iter);

  // Map non-known size
  int idx = 0;
  *iter = (iter_t) {
    .opt = 0,
    .data = &idx,
    .next = nextUpTo10,
    .type_size = sizeof(int),
    .free = NULL,
  };
  arr = array_create(sizeof(int));

  iter_map(iter, arr, addOne);

  for (int i = 0; i < 9; i++)
    assert(INTVAL(array_get(arr, i)) == i + 2);
  assert(arr->size == 9);

  array_destroy(arr);
  iter_destroy(iter);

  // reduce
  arr = array_create(sizeof(int));
  for (int i = 0; i < 10; i++)
    array_push(arr, &i);
  iter = array_createIterator(arr);

  int i;
  iter_reduce(iter, &i, summing);
  assert(i == 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9);

  iter_destroy(iter);

  // max
  iter = array_createIterator(arr);

  assert(INTVAL(iter_max(iter, intCmp)) == 9);

  iter_destroy(iter);
  array_destroy(arr);

  return 0;
}
