#include "include/btree.h"
#include "include/pool_alloc.h"
#include "include/utils.h"
#include "include/vector.h"
#include "include/qsort.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

VECTOR_IMPL(bt_kp, kp)

unsigned find_sorted_slice(bt_kp *data, unsigned start, unsigned end, int key) {
  int l = start;
  int h = end;
  int out = -1;

  while (h > l) {
    int m = l + (h - l) / 2;
    int val = data[m].key;
    if (val < key) {
      l = m + 1;
    } else if (val > key) {
      h = m;
    } else {
      out = m;
      h = m;
    }
  }

  if (out == -1) {
    out = h - 1;
  }

  if (key > data[out].key) {
    out++;
  }

  return out;

}

unsigned find_sorted(vec_kp data, int key) {
  return find_sorted_slice(data.data, 0, data.size, key);
}

unsigned leaf_search(int key, btree *bt, bt_leaf *leaf) {
#ifdef LEAF_BINARY_SEARCH
  return find_sorted_slice(bt->base_data.data, leaf->start, leaf->end, key);
#else
  unsigned k = 0;
  while ((k < size) && (keys[k].key < key)) {
    ++k;
  }
  return k;
#endif
}

unsigned leaf_search_lt(int key, btree *bt, bt_leaf *leaf) {
#ifdef LEAF_BINARY_SEARCH
  int l = leaf->start - 1;
  int r = leaf->end;
  int m;

  while (r - l > 1) {
    m = (l + r) / 2;
    if (bt->base_data.data[m].key > key) {
      r = m;
    } else {
      l = m;
    }
  }

  return (unsigned)r;
#else
  // unsigned k = 0;
  // while ((k < size) && (keys[k].key < key)) {
  //   ++k;
  // }
  // return k;
#endif
}

unsigned internal_search(int key, int *keys, unsigned size) {
#ifdef INTERNAL_BINARY_SEARCH
  int l = -1;
  int r = size;
  int m;

  while (r - l > 1) {
    m = (l + r) / 2;
    if (keys[m] <= key) {
      l = m;
    } else {
      r = m;
    }
  }

  return (unsigned)r;
#else
  unsigned k = 0;
  while ((k < size) && (keys[k] <= key)) {
    ++k;
  }
  return k;
#endif
}

bt_internal *create_internal(btree *bt) {
  bt_internal *node = malloc_chunk(bt->internal_alloc);
  node->size = 0;
  return node;
}

bt_leaf *create_leaf(btree *bt) {
  bt_leaf *node = malloc_chunk(bt->leaf_alloc);
  node->start = 0;
  node->end = 0;
  return node;
}

void destroy_leaf(btree *bt, bt_leaf *leaf) {
  free_chunk(bt->leaf_alloc, leaf);
}

void destroy_internal(btree *bt, bt_internal *internal) {
  free_chunk(bt->internal_alloc, internal);
}

static inline int cmp(const void *a, const void *b) {
  return ((*(bt_kp *)a).key - (*(bt_kp *)b).key);
}

arr_kp to_arr_kp(vec_int base) {
  arr_kp base_kp;
  alloc_arr_kp(&base_kp, base.size);
  for (size_t i = 0; i < base.size; ++i) {
    bt_kp kp;
    kp.key = get_vec_int(&base, i);
    kp.pos = i;
    set_arr_kp(&base_kp, kp, i);
  }
  return base_kp;
}

void load_btree(btree *bt, vec_kp base_kp) {
  // Step 3: Assemble the leaves
  vec_ptr leaves;
  alloc_vec_ptr(&leaves, base_kp.size / LEAF_FANOUT + 1);

  size_t i = 0;
  bt_leaf *prev = NULL;
  while (i < base_kp.size) {
    bt_leaf *leaf = create_leaf(bt);
    leaf->start = i;
    leaf->end = leaf->start + min(base_kp.size - i, LEAF_FANOUT);

    leaf->low = bt->base_data.data[leaf->start].key;
    leaf->high = bt->base_data.data[leaf->end - 1].key;
    i = leaf->end;

    // printf("leaf node [%d, %d] size %u\n", leaf->low, leaf->high,
    // leaf->size);

    if (prev != NULL) {
      prev->next = leaf;
    }

    push_vec_ptr(&leaves, leaf);
  }

  // Step 4: Build the internal nodes
  bool leaf_level = true;
  vec_ptr lower_level = leaves;
  while (lower_level.size > 1) {
    bt->depth++;
    vec_ptr current_level;
    alloc_vec_ptr(&current_level, lower_level.size / INTERNAL_FANOUT + 1);

    bt_internal *current = create_internal(bt);
    for (size_t i = 0; i < lower_level.size; ++i) {
      void *node = get_vec_ptr(&lower_level, i);

      // If full, push to node list
      if (current->size == INTERNAL_FANOUT + 1) {
        // printf("internal depth %d node [%d, %d] size %u\n", bt->depth,
        //        current->keys[0], current->keys[current->size - 2],
        //        current->size);
        push_vec_ptr(&current_level, current);

        if (i != lower_level.size - 1) {
          current = create_internal(bt);
        }
      }

      // Set keys
      if (current->size < INTERNAL_FANOUT) {
        if (leaf_level) {
          bt_leaf *leaf = node;
          current->keys[current->size] = leaf->high;
        } else {
          bt_internal *internal = node;
          current->keys[current->size] = internal->high;
        }
      }

      // Calculate min and max
      if (leaf_level) {
        bt_leaf *leaf = node;
        current->low = min(current->low, leaf->low);
        current->high = max(current->high, leaf->high);
      } else {
        bt_internal *internal = node;
        current->low = min(current->low, internal->low);
        current->high = max(current->high, internal->high);
      }

      current->children[current->size] = node;
      current->size++;
    }
    // printf("internal depth %d node [%d, %d] size %u\n", bt->depth,
    //        current->low, current->high, current->size);
    push_vec_ptr(&current_level, current);

    if (leaf_level) {
      leaf_level = false;
    }

    // Free and jump level up
    free_vec_ptr(&lower_level);
    lower_level = current_level;
  }

  // Step 5: Get the root
  assert(lower_level.size == 1);
  bt->root = get_vec_ptr(&lower_level, 0);
  free_vec_ptr(&lower_level);
}

btree *create_btree(vec_kp base_kp, bool sort) {
  btree *bt = calloc(1, sizeof(btree));
  bt->internal_alloc = new_pool_alloc(sizeof(bt_internal), 128, 128);
  bt->leaf_alloc = new_pool_alloc(sizeof(bt_leaf), 128, 128);
  bt->base_data = base_kp;

  // // Step 1: Reorganize the data
  // arr_kp base_kp = to_arr_kp(*base);
  //
  if (sort) {
    // Step 2: Sort the data
    QSORT(bt_kp, base_kp.data, base_kp.size, cmp);
  }

  // Step 3-5
  load_btree(bt, base_kp);

  return bt;
}

void destroy_btree(btree *bt) {
  free_vec_kp(&bt->base_data);
  delete_pool_alloc(bt->leaf_alloc);
  delete_pool_alloc(bt->internal_alloc);
  free(bt);
}

void insert(btree *bt, bt_kp key_pos) {
  (void)bt;
  (void)key_pos;
  assert(false);
}

bt_leaf *find_leaf(btree *bt, int key) {
  if (bt == NULL) {
    return false;
  }

  void *node = bt->root;
  unsigned depth = bt->depth;
  unsigned index = 0;

  while (depth-- > 0) {
    bt_internal *internal = node;
    index = internal_search(key, &internal->keys[0],
                            min(internal->size - 1, INTERNAL_FANOUT));
    node = internal->children[index];
  }

  bt_leaf *leaf = node;
  return leaf;
}

bool find_first(btree *bt, int key, bt_kp *out) {
  bt_leaf *leaf = find_leaf(bt, key);
  unsigned index = leaf_search(key, bt, leaf);
  if (bt->base_data.data[index].key == key) {
    if (out != NULL) {
      *out = bt->base_data.data[index];
      return true;
    }
  }

  return false;
}

// TODO: performance
bool find_last(btree *bt, int key, bt_kp *out) {
  bt_leaf *leaf = find_leaf(bt, key);
  unsigned index = leaf_search(key, bt, leaf);

  if (bt->base_data.data[index].key != key) {
    return false;
  }

  if (out == NULL) {
    return false;
  }

  while (leaf != NULL && bt->base_data.data[index].key == key) {
    *out = bt->base_data.data[index];

    if (index == leaf->end - 1) {
      leaf = leaf->next;
      index = 0;
    } else {
      ++index;
    }
  }

  if (out->key == key) {
    return true;
  }

  return false;
}

unsigned find(btree *bt, int key) {
  bt_leaf *leaf = find_leaf(bt, key);
  return leaf_search(key, bt, leaf);
}
