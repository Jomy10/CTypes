#define CT_ARRAY_IMPL
#include "../CArray.h"
#include <assert.h>

int int_cmp(const void* a, const void* b) {
  return (*(int*)a - *(int*)b);
}

int main() {
  int val;
  Array(int) arr = array_create(sizeof(int));
  assert(!array_hasIndex(arr, 0));
  assert(!array_first(arr));
  assert(!array_last(arr));
  val = 1;
  assert(!array_push(arr, &val));
  assert(array_hasIndex(arr, 0));
  assert(!array_hasIndex(arr, 1));
  assert(*((int*)array_first(arr)) == val);
  assert(*((int*)array_last(arr)) == val);
  val = 10;
  array_set(arr, 0, &val);
  assert(*((int*)array_first(arr)) == val);
  assert(arr->size == 1);

  assert(*((int*)array_get(arr, 0)) == val);
  assert(array_getChecked(arr, 1) == NULL);

  val = 15;
  assert(!array_pushFirst(arr, &val));
  assert(*((int*)array_getChecked(arr, 0)) == val);
  assert(*((int*)array_getChecked(arr, 1)) == 10);

  assert(array_pop(arr, &val) == 1);
  assert(val == 10);

  array_reset(arr);

  for (int i = 0; i < 25; i++) {
    array_push(arr, &i);
  }
  assert(array_popAt(arr, 10, &val) == 24);
  assert(val == 10);

  assert(array_popFirst(arr, &val) == 23);
  assert(val == 0);

  assert(array_pop(arr, &val) == 22);
  assert(val == 24);

  array_swap(arr, 0, 1);
  assert(*((int*)array_get(arr, 0)) == 2);
  assert(*((int*)array_get(arr, 1)) == 1);

  array_sort(arr, int_cmp, qsort);
  assert(*((int*)array_get(arr, 0)) == 1);
  assert(*((int*)array_get(arr, 1)) == 2);

  array_reverse(arr);
  assert(*((int*)array_get(arr, 0)) == 23);
  assert(*((int*)array_get(arr, arr->size - 1)) == 1);

  array_destroy(arr);

  return 0;
}
