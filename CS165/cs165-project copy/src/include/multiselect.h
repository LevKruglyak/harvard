#pragma once

#include "globals.h"
#include "multiselect.h"
#include "operators.h"
#include "thread_pool.h"
#include "vector.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct {
  int low;
  int high;
} full_filter;

VECTOR_HEAD(full_filter, filter)
ARRAY_HEAD(full_filter, filter)

typedef struct {
  int *data;
  size_t start;
  size_t end;
  size_t index;
} column_chunk;

typedef struct {
#ifdef ENABLE_MULTITHREADING
  pthread_mutex_t lock;
  arr_p_vec_index intermediates;
  size_t remaining;
#endif
  SelectVariable *select;
  full_filter filter;
} select_result;

VECTOR_HEAD(select_result, sel_res)
ARRAY_HEAD(select_result, sel_res)

VECTOR_HEAD(column_chunk, col_chunk)
ARRAY_HEAD(column_chunk, col_chunk)

typedef struct {
  pthread_mutex_t lock;
  unsigned count;
  arr_sel_res selection_results;
  vec_col_chunk col_chunks;
} _multiselect_arc;

typedef struct {
  column_chunk *col;
  select_result *sel;
  _multiselect_arc *_arc;
} work_chunk;

full_filter convert_filter(Filter filter);
void multiselect(thread_pool *pool, vec_int column, arr_filter filters,
                 arr_sel_var selections);

typedef struct {
  size_t max_col_partition_size;
  // Dependant on number of queries
  bool block_on_columns;
  bool block_within_select;
} multiselect_tuning;
