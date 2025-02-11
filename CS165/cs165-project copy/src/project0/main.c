#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "string_hashtable.h"

// This is where you can implement your own tests for the hash table
// implementation.
int main(void) {

  string_hashtable *ht = NULL;
  int size = 1000;
  assert(strh_allocate(&ht, size) == 0);

  int test[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  printf("data: %p\n", test);

  strh_put(ht, "hello", &test[0]);
  strh_put(ht, "world", &test[1]);

  printf("ht[hello] -> %p\n", strh_get(ht, "hello"));
  printf("ht[world] -> %p\n", strh_get(ht, "world"));
  printf("ht[test] -> %p\n", strh_get(ht, "test"));

  strh_deallocate(ht);
  return 0;
}
