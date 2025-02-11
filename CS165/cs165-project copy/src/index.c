#include "include/btree.h"
#include "include/db_internals.h"
#include "include/db_types.h"
#include "include/globals.h"
#include "include/qsort.h"
#include "include/string_hashtable.h"
#include <limits.h>
#include <stdlib.h>

VECTOR_IMPL(col_copy, col_copy)

static inline int cmp_grp(const void *a, const void *b) {
  return (((int *)a)[0] - ((int *)b)[0]);
}

static inline int cmp_kp(const void *a, const void *b) {
  return ((*(bt_kp *)a).key - (*(bt_kp *)b).key);
}

clustered_index_data multisort(Column *col) {
  clustered_index_data data;

  // TODO: Optimize

  // Step 1: group into large tuple
  unsigned col_count = col->tbl->columns_count;
  int *row_store = malloc(sizeof(int) * col_count * col->data.size);
  vec_col cols;
  alloc_vec_col(&cols, col_count);
  // Make sure primary col is first
  push_vec_col(&cols, col);
  for (size_t i = 0; i < col_count; i++) {
    Column *current_col = get_vec_col(col->tbl->ordered_columns, i);
    if (col != current_col) {
      push_vec_col(&cols, current_col);
    }
  }

  for (size_t i = 0; i < col->data.size; ++i) {
    for (size_t j = 0; j < col_count; j++) {
      Column *current_col = get_vec_col(&cols, j);
      row_store[j + i * col_count] = get_vec_int(&current_col->data, i);
    }
  }

  // Step 2: Sort
  qsort(row_store, col->data.size, col_count * sizeof(int), cmp_grp);

  // Step 3: Put stuff back
  alloc_arr_col_copy(&data.copies, col_count);
  for (size_t i = 0; i < col_count; i++) {
    Column *current_col = get_vec_col(&cols, i);

    col_copy copy;
    copy.col = current_col;
    alloc_vec_int(&copy.data, current_col->data.size);

    for (size_t j = 0; j < current_col->data.size; j++) {
      set_vec_int(&copy.data, row_store[i + j * col_count], j);
    }
    copy.data.size = current_col->data.size;

    set_arr_col_copy(&data.copies, copy, current_col->col_id);
  }
  free(row_store);
  free_vec_col(&cols);

  return data;
}

idx *create_index(index_form form, index_type type, Column *col) {
  (void)col;
  idx *index = malloc(sizeof(idx));
  index->type = type;
  index->form = form;
  index->initialized = false;
  return index;
}

void initialize_index(idx *index, Column *col) {
  index->initialized = true;

  if (index->type == CLUSTERED) {
    index->cluster_data = multisort(col);
  }

  // TODO: redundant for clustered indices
  arr_kp sorted_data = to_arr_kp(col->data);
  QSORT(bt_kp, sorted_data.data, sorted_data.size, cmp_kp);

  if (index->form == BTREE) {
    index->form_data.btree = create_btree(to_vec_kp(sorted_data), false);
  } else {
    index->form_data.sorted = to_vec_kp(sorted_data);
  }
}

void destroy_index(idx *index) {
  if (index->type == CLUSTERED) {
    clustered_index_data data = index->cluster_data;
    for (size_t i = 0; i < data.copies.size; ++i) {
      col_copy copy = get_arr_col_copy(&data.copies, i);

      // TODO: mmap
      free_vec_int(&copy.data);
    }
    free_arr_col_copy(&data.copies);
  }

  if (index->form == BTREE) {
    destroy_btree(index->form_data.btree);
  } else {
    free_vec_kp(&index->form_data.sorted);
  }

  free(index);
}
