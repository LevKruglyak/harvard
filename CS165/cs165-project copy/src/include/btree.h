#pragma once

#include <stdbool.h>
#include "pool_alloc.h"
#include "vector.h"

// Performance
#define LEAF_BINARY_SEARCH
// #define INTERNAL_BINARY_SEARCH

typedef struct {
  int key;
  unsigned pos;
} bt_kp;

VECTOR_HEAD(bt_kp, kp)
ARRAY_HEAD(bt_kp, kp)

#define INTERNAL_FANOUT 64

// Make sure leaf fits into a 4k page
#define LEAF_FANOUT 4096

typedef struct bt_leaf {
  unsigned start;
  unsigned end;
  struct bt_leaf *next;
  int low;
  int high;
  // char _pad[8];
} bt_leaf;

typedef struct {
  unsigned size;
  int keys[INTERNAL_FANOUT];
  void *children[INTERNAL_FANOUT + 1];
  int low;
  int high;
  char _pad[8];
} bt_internal;

typedef struct {
  void *root;
  unsigned depth;
  vec_kp base_data;
  // Node allocators
  pool_alloc *leaf_alloc;
  pool_alloc *internal_alloc;
} btree;

btree *create_btree(vec_kp base_kp, bool sort);
void destroy_btree(btree *btree);

void insert(btree *bt, bt_kp key_pos);

arr_kp to_arr_kp(vec_int data);

// Find the first instance of the key in the btree
bool find_first(btree *bt, int key, bt_kp *out);
// Find the last instance of the key in the btree
bool find_last(btree *bt, int key, bt_kp *out);

unsigned find(btree *bt, int key);
unsigned find_sorted(vec_kp data, int key);
