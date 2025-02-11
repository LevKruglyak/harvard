#include "include/btree.h"
#include "include/vector.h"
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static inline void DoNotOptimize(int value) {
  __asm__ __volatile__("" : "+r,m"(value) : : "memory");
}

int binary_search(int key, vec_int data) {
  int l = -1;
  int r = data.size;
  int m;

  while (r - l > 1) {
    m = (l + r) / 2;
    if (data.data[m] <= key) {
      l = m;
    } else {
      r = m;
    }
  }

  return m;
}

#define START_TIME(func)                                                       \
  struct timeval start_##func;                                                 \
  gettimeofday(&start_##func, NULL);

#define END_TIME(func, name)                                                   \
  struct timeval end_##func;                                                   \
  gettimeofday(&end_##func, NULL);                                             \
  printf(name " took %.3f ms\n",                                               \
         1000.0 * (end_##func.tv_sec - start_##func.tv_sec) +                  \
             (float)(end_##func.tv_usec - start_##func.tv_usec) / 1000.0);

int main() {
  size_t data_size = 20 * 1024 * 1024;

  vec_int data;
  alloc_vec_int(&data, data_size);
  for (size_t i = 0; i < data_size; ++i) {
    push_vec_int(&data, rand() % data_size);
  }

  START_TIME(build_btree)
  btree *bt = create_btree(to_vec_kp(to_arr_kp(data)), true);
  printf("built tree of depth %d\n", bt->depth);
  END_TIME(build_btree, "build")

  long a = 0;

  START_TIME(binary_search)
  for (int i = 0; i < 1000; ++i) {
    a += binary_search(i, data);
  }
  END_TIME(binary_search, "bin_search")

  START_TIME(btree_search)
  for (int i = 0; i < 1000; ++i) {
    bt_kp kp;
    a += find_first(bt, i, &kp);
  }
  END_TIME(btree_search, "btree_search")

  for (size_t i = 0; i < 10; ++i) {
    int key = rand() % 100;
    printf("key %d\n", key);
    bt_kp pv;
    if (find_first(bt, key, &pv)) {
      printf("first search for %d: %d\n", key, pv.pos);
      assert(key == data.data[pv.pos]);
    }

    if (find_last(bt, key, &pv)) {
      printf("last search for %d: %d\n", key, pv.pos);
      assert(key == data.data[pv.pos]);
      printf("\n");
    }
  }

  free_vec_int(&data);
  destroy_btree(bt);
}
