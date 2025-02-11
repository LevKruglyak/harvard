#include "include/multiselect.h"
#include "include/batch.h"
#include "include/rand.h"
#include "include/stats.h"
#include "include/thread_pool.h"
#include "include/types.h"
#include "include/utils.h"
#include "include/vector.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

VECTOR_IMPL(full_filter, filter)
VECTOR_IMPL(column_chunk, col_chunk)

full_filter convert_filter(Filter filter) {
  full_filter result;

  if (is_some_int(filter.high)) {
    result.high = unwrap_int(filter.high);
  } else {
    result.high = INT_MAX;
  }

  if (is_some_int(filter.low)) {
    result.low = unwrap_int(filter.low);
  } else {
    result.low = -INT_MAX;
  }

  return result;
}

// [CRITICAL] performance
void internal_select(column_chunk col, full_filter filter, vec_index *select,
                     bool inverse) {
  if (inverse) {
    for (size_t i = col.start; i < col.end; i++) {
      int data = col.data[i];
      if ((data < filter.low) | (data >= filter.high)) {
        fast_push_vec_index(select, i, SELECT_GROWTH_RATE);
      }
    }
  } else {
    for (size_t i = col.start; i < col.end; i++) {
      int data = col.data[i];
      if ((data >= filter.low) & (data < filter.high)) {
        fast_push_vec_index(select, i, SELECT_GROWTH_RATE);
      }
    }
  }
}

void execute_multiselect_chunk(column_chunk col, select_result sel) {
  internal_select(col, sel.filter, &sel.select->selection, sel.select->inverse);
}

void free_arc(_multiselect_arc *_arc) {
  pthread_mutex_lock(&_arc->lock);
  _arc->count--;

  if (_arc->count == 0) {
    free_vec_col_chunk(&_arc->col_chunks);
    free_arr_sel_res(&_arc->selection_results);
    pthread_mutex_destroy(&_arc->lock);
    free(_arc);
  } else {
    pthread_mutex_unlock(&_arc->lock);
  }
}

#ifdef ENABLE_MULTITHREADING
void thread_execute_multiselect_chunk(void *arg) {
  work_chunk *chunk = arg;
  column_chunk *col = chunk->col;
  select_result *sel = chunk->sel;

  // Alocate temporary selection
  alloc_vec_index(&sel->intermediates.data[col->index],
                  INITIAL_SELECTION_CAPACITY);

  // Step 1: perform local selection
  internal_select(*col, sel->filter, &sel->intermediates.data[col->index],
                  sel->select->inverse);

  // [CRITICAL] performance
  // Step 2: copy data to global selection (TODO: improve)
  pthread_mutex_lock(&sel->lock);
  sel->remaining--;
  if (sel->remaining == 0) {
    // Perform merge
    for (size_t i = 0; i < sel->intermediates.size; ++i) {
      append_vec_index(&sel->select->selection, &sel->intermediates.data[i]);
      free_vec_index(&sel->intermediates.data[i]);
    }
    free_arr_p_vec_index(&sel->intermediates);
    pthread_mutex_destroy(&sel->lock);
  } else {
    pthread_mutex_unlock(&sel->lock);
  }

  free_arc(chunk->_arc);
  free(arg);
}

void thread_execute_multiselect_chunk_without_intermediates(void *arg) {
  work_chunk *chunk = arg;
  column_chunk *col = chunk->col;
  select_result *sel = chunk->sel;

  pthread_mutex_lock(&sel->lock);
  internal_select(*col, sel->filter, &sel->select->selection,
                  sel->select->inverse);
  sel->remaining--;
  if (sel->remaining == 0) {
    free_arr_p_vec_index(&sel->intermediates);
    pthread_mutex_destroy(&sel->lock);
  } else {
    pthread_mutex_unlock(&sel->lock);
  }

  free_arc(chunk->_arc);
  free(arg);
}
#endif

multiselect_tuning decide_tuning_params(vec_int column, arr_filter filters,
                                        arr_sel_var selections) {
  multiselect_tuning mt;
  mt.max_col_partition_size = MAX_COLUMN_PARTITION_SIZE;
  mt.block_on_columns = false;
  // TODO: not sure if improves performance on benchmark
  mt.block_within_select =
      selections.size >= (size_t)(number_of_processors() / 2);
  if (mt.block_within_select) {
    printf("blocking within select\n");
  }

#ifdef USE_SELECTIVITY_ESTIMATOR
  struct timeval start;
  gettimeofday(&start, NULL);
  // Monte-Carlo estimate of filter selectivity
  mt_rand rand = m_srand(time(NULL));
  arr_flt selectivities;
  alloc_arr_flt(&selectivities, filters.size);
  for (size_t i = 0; i < filters.size; ++i) {
    set_arr_flt(&selectivities, 0.0, i);
  }

  size_t num_samples =
      column.size * sizeof(int) / (10 * MIN_COLUMN_PARTITION_CHUNK_SIZE);
  num_samples = min(num_samples, MAX_SELECTIVITY_ESTIMATOR_SAMPLES);

  for (size_t i = 0; i < num_samples; ++i) {
    size_t page_index = m_gen_rand(&rand) % column.size;
    int *page = &column.data[page_index];
    size_t page_size = min(SELECTIVITY_ESTIMATOR_PAGE_SIZE / sizeof(int),
                           column.size - page_index);

    for (size_t j = 0; j < filters.size; ++j) {
      full_filter filter = get_arr_filter(&filters, j);
      float selectivity = 0.0;
      for (size_t k = 0; k < page_size; ++k) {
        int data = page[k];
        if ((filter.low <= data) & (filter.high > data)) {
          selectivity++;
        }
      }

      selectivity /= (float)page_size;
      float total_selectivity = get_arr_flt(&selectivities, j);
      set_arr_flt(&selectivities,
                  total_selectivity + selectivity / (float)num_samples, j);
    }
  }

  struct timeval end;
  gettimeofday(&end, NULL);

  if (num_samples > 0) {
    printf("time to measure selectivity %lu us using %lu samples\n",
           1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec,
           num_samples);

    for (size_t i = 0; i < filters.size; ++i) {
      full_filter filter = get_arr_filter(&filters, i);
      float selectivity = get_arr_flt(&selectivities, i);

      if (selectivity > 0.5) {
        get_arr_sel_var(&selections, i)->inverse = true;
      }

      // if (selectivity > 0.95) {
      // get_arr_sel_var(&selections, i)->no_block = true;
      // }

      printf("selectivity %f for filter [%d, %d]\n", selectivity, filter.low,
             filter.high);
    }
  }

  free_arr_flt(&selectivities);
#endif

  return mt;
}

void multiselect(thread_pool *pool, vec_int column, arr_filter filters,
                 arr_sel_var selections) {
  assert(filters.size == selections.size);

  // Step 1: Tune algorithm to perform optimally
  multiselect_tuning mt = decide_tuning_params(column, filters, selections);

  // Step 3: Break up column into chunks and execute
  vec_col_chunk col_chunks;
  alloc_vec_col_chunk(&col_chunks, mt.max_col_partition_size);
  {
    size_t number_of_chunks =
        min(sizeof(int) * column.size / MIN_COLUMN_PARTITION_CHUNK_SIZE,
            mt.max_col_partition_size);
    number_of_chunks = max(number_of_chunks, 1);
    size_t chunk_size = column.size / number_of_chunks;
    column_chunk chunk;
    chunk.start = 0;
    chunk.index = 0;
    chunk.end = min(chunk_size, column.size);
    chunk.data = column.data;

    while (chunk.end <= column.size) {
      push_vec_col_chunk(&col_chunks, chunk);
      if (chunk.end == column.size) {
        break;
      }

      chunk.start = chunk.end;
      chunk.index++;
      chunk.end = min(chunk.start + chunk_size, column.size);
    }
  }

  printf("using %lu column chunks\n", col_chunks.size);

  // Step 2: Set up synchronization for selections
  // (todo: sort)
  arr_sel_res selection_results;
  alloc_arr_sel_res(&selection_results, selections.size);

  for (size_t i = 0; i < selections.size; ++i) {
    select_result result;
    result.select = get_arr_sel_var(&selections, i);
    result.filter = get_arr_filter(&filters, i);
#ifdef ENABLE_MULTITHREADING
    pthread_mutex_init(&result.lock, NULL);
    alloc_arr_p_vec_index(&result.intermediates, col_chunks.size);
    result.remaining = col_chunks.size;
#endif
    set_arr_sel_res(&selection_results, result, i);
  }

#ifdef ENABLE_MULTITHREADING
  // Step 3: Create async reference counted pointer to prevent leaks
  _multiselect_arc *_arc = malloc(sizeof(_multiselect_arc));
  _arc->count = col_chunks.size * selection_results.size;
  _arc->col_chunks = col_chunks;
  _arc->selection_results = selection_results;
  pthread_mutex_init(&_arc->lock, NULL);
#endif

  // Step 4: Generate work chunks and pass to thread pool
  for (size_t i = 0; i < col_chunks.size; ++i) {
    for (size_t j = 0; j < selection_results.size; ++j) {
#ifndef ENABLE_MULTITHREADING
      execute_multiselect_chunk(get_vec_col_chunk(&col_chunks, i),
                                get_arr_sel_res(&selection_results, j));
    }
#endif

#ifdef ENABLE_MULTITHREADING
    work_chunk *chunk = malloc(sizeof(work_chunk));
    chunk->col = &col_chunks.data[i];
    chunk->sel = &selection_results.data[j];
    chunk->_arc = _arc;

    if (mt.block_on_columns |
        (mt.block_within_select & !chunk->sel->select->no_block)) {
      add_task_thread_pool(
          pool, thread_execute_multiselect_chunk_without_intermediates, chunk);
    } else {
      add_task_thread_pool(pool, thread_execute_multiselect_chunk, chunk);
    }
  }

  if (mt.block_on_columns) {
    wait_thread_pool(pool);
  }
#endif
}

#ifndef ENABLE_MULTITHREADING
free_vec_col_chunk(&col_chunks);
free_arr_sel_res(&selection_results);
#endif
}
