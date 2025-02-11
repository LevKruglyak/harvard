#include "include/db_internals.h"
#include "include/globals.h"
#include "include/join_hashtable.h"
#include "include/thread_pool.h"
#include "include/types.h"
#include "include/utils.h"
#include "include/vector.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

// [CRITICAL] performance
void select_range(vec_int data, vec_index *result, opt_int lower,
                  opt_int upper) {
  int low = is_some_int(lower) ? unwrap_int(lower) : -INT_MAX;
  int high = is_some_int(upper) ? unwrap_int(upper) : INT_MAX;

  for (size_t i = 0; i < data.size; ++i) {
    int current_data = get_vec_int(&data, i);
    if ((current_data >= low) & (current_data < high)) {
      fast_push_vec_index(result, i, SELECT_GROWTH_RATE);
    }
  }
}

// [CRITICAL] performance
void reselect(vec_int data, vec_index original, vec_index *result,
              opt_int lower, opt_int higher) {
  assert(data.size == original.size);
  vec_index *new_selection = new_vec_index(INITIAL_SELECTION_CAPACITY);
  select_range(data, new_selection, lower, higher);

  for (size_t i = 0; i < new_selection->size; ++i) {
    fast_push_vec_index(
        result, get_vec_index(&original, get_vec_index(new_selection, i)),
        SELECT_GROWTH_RATE);
  }
  delete_vec_index(new_selection);
}

// [CRITICAL] performance
void fetch(vec_int data, vec_index select, arr_int *fetch) {
  // Just in case
  free_arr_int(fetch);

  alloc_arr_int(fetch, select.size);
  for (size_t i = 0; i < select.size; ++i) {
    set_arr_int(fetch, get_vec_int(&data, get_vec_index(&select, i)), i);
  }
}

// [CRITICAL] performance
void fetch_index_range(vec_int data, unsigned low, unsigned high,
                       arr_int *fetch) {
  // Just in case
  free_arr_int(fetch);

  alloc_arr_int(fetch, high - low);
  for (size_t i = low; i < high; ++i) {
    set_arr_int(fetch, get_vec_int(&data, i), i - low);
  }
}

// [CRITICAL] performance
void reselect_inverse(vec_int data, vec_index original, size_t total,
                      vec_index *result, opt_int lower, opt_int higher) {
  (void)data;
  (void)original;
  (void)total;
  (void)result;
  (void)lower;
  (void)higher;

  assert(data.size == original.size);
  vec_index *new_selection = new_vec_index(INITIAL_SELECTION_CAPACITY);
  select_range(data, new_selection, lower, higher);

  for (size_t i = 0; i < new_selection->size; ++i) {
    assert(false);
  }

  delete_vec_index(new_selection);
}

// [CRITICAL] performance
void fetch_inverse(vec_int data, vec_index select, arr_int *fetch) {
  // Just in case
  free_arr_int(fetch);
  alloc_arr_int(fetch, data.size - select.size);

  if (select.size == 0) {
    memcpy(fetch->data, data.data, data.size * sizeof(int));
  } else {
    size_t j = 0;
    size_t next_ignore = get_vec_index(&select, 0);
    for (size_t i = 0; i < data.size; ++i) {

      if ((j < select.size) & (i == next_ignore)) {
        next_ignore = get_vec_index(&select, ++j);
      } else {
        set_arr_int(fetch, i - j, i);
      }
    }
  }
}

// [CRITICAL] performance
double avg(vec_int data) {
  if (data.size == 0) {
    return 0.0;
  }

  return (long double)sum(data) / data.size;
}

// [CRITICAL] performance
int _min(vec_int data) {
  int min = INT_MAX;
  for (size_t i = 0; i < data.size; ++i) {
    int value = get_vec_int(&data, i);
    if (value < min) {
      min = value;
    }
  }
  return min;
}

// [CRITICAL] performance
int _max(vec_int data) {
  int max = -INT_MAX;
  for (size_t i = 0; i < data.size; ++i) {
    int value = get_vec_int(&data, i);
    if (value > max) {
      max = value;
    }
  }
  return max;
}

// [CRITICAL] performance
long long sum(vec_int data) {
  long long res = 0;
  for (size_t i = 0; i < data.size; ++i) {
    res += (long)get_vec_int(&data, i);
  }
  return res;
}

// [CRITICAL] performance
void add(vec_int first, vec_int second, arr_int *result) {
  assert(first.size == second.size);
  alloc_arr_int(result, first.size);

  for (size_t i = 0; i < first.size; ++i) {
    set_arr_int(result, get_vec_int(&first, i) + get_vec_int(&second, i), i);
  }
}

// [CRITICAL] performance
void sub(vec_int first, vec_int second, arr_int *result) {
  assert(first.size == second.size);
  alloc_arr_int(result, first.size);

  for (size_t i = 0; i < first.size; ++i) {
    set_arr_int(result, get_vec_int(&first, i) - get_vec_int(&second, i), i);
  }
}

SWAP_IMPL(arr_int, arr_int)
SWAP_IMPL(vec_index, vec_index)
SWAP_IMPL(vec_index *, vec_index_ptr)

// [CRITICAL] performance
void nested_loop_join(arr_int first_fetch, vec_index first_selection,
                      arr_int second_fetch, vec_index second_selection,
                      JoinResult *res) {
  if (first_selection.size > second_selection.size) {
    swap_arr_int(&first_fetch, &second_fetch);
    swap_vec_index(&first_selection, &second_selection);
    swap_vec_index_ptr(&res->first, &res->second);
  }

  // (block) nested loop join
  // size_t first_block_count = (first_selection.size + 1) /
  // (NESTED_LOOP_BLOCK_SIZE /sizeof(int)); size_t second_block_count =
  // (second_selection.size + 1) / (NESTED_LOOP_BLOCK_SIZE /sizeof(int));
  // printf("nested join %lu x %lu\n", first_block_count, second_block_count);
  //
  // for (size_t block_i = 0; block_i < first_block_count; ++block_i) {
  //   for (size_t block_j = 0; block_j < second_block_count; ++block_j) {
  //     for (size_t i = block_i * NESTED_LOOP_BLOCK_SIZE;
  //          i <
  //          min((block_i + 1) * NESTED_LOOP_BLOCK_SIZE, first_selection.size);
  //          ++i) {
  //       for (size_t j = block_j * NESTED_LOOP_BLOCK_SIZE;
  //            j <
  //            min((block_j + 1) * NESTED_LOOP_BLOCK_SIZE,
  //            second_selection.size);
  //            ++j) {
  //         if (get_arr_int(&first_fetch, i) == get_arr_int(&second_fetch, j))
  //         {
  //           push_vec_index(res->first, get_vec_index(&first_selection, i));
  //           push_vec_index(res->second, get_vec_index(&second_selection, j));
  //         }
  //       }
  //     }
  //   }
  // }

  // nested loop join
  for (size_t i = 0; i < first_selection.size; ++i) {
    for (size_t j = 0; j < second_selection.size; ++j) {
      if (get_arr_int(&first_fetch, i) == get_arr_int(&second_fetch, j)) {
        push_vec_index(res->first, get_vec_index(&first_selection, i));
        push_vec_index(res->second, get_vec_index(&second_selection, j));
      }
    }
  }
}

// [CRITICAL] performance
void hash_join(arr_int first_fetch, vec_index first_selection,
               arr_int second_fetch, vec_index second_selection,
               JoinResult *res) {
  if (first_selection.size > second_selection.size) {
    swap_arr_int(&first_fetch, &second_fetch);
    swap_vec_index(&first_selection, &second_selection);
    swap_vec_index_ptr(&res->first, &res->second);
  }

  jh_table *table;
  jh_allocate(&table, first_selection.size, true);

  // Populate the hash table
  for (size_t i = 0; i < first_selection.size; ++i) {
    jh_put(table, get_arr_int(&first_fetch, i),
           get_vec_index(&first_selection, i));
  }

  // Perform join
  vec_jh_val first_vals;
  alloc_vec_jh_val(&first_vals, 10);
  for (size_t j = 0; j < second_selection.size; ++j) {
    int data = get_arr_int(&second_fetch, j);

    jh_fast_get(table, data, &first_vals);
    for (size_t i = 0; i < first_vals.size; ++i) {
      fast_push_vec_index(res->first, get_vec_jh_val(&first_vals, i), 2);
      fast_push_vec_index(res->second, get_vec_index(&second_selection, j), 2);
    }
  }
  free_vec_jh_val(&first_vals);

  jh_deallocate(table);
}

typedef struct {
  arr_int *fetch;
  vec_index *select;
  jh_table **table;
  size_t start;
  size_t end;
} fast_hash_params;

void fast_hash_join_task(void *args) {
  fast_hash_params params = *(fast_hash_params *)args;
  jh_allocate(params.table, params.end - params.start, true);

  for (size_t i = params.start; i < params.end; ++i) {
    jh_put(*params.table, get_arr_int(params.fetch, i),
           get_vec_index(params.select, i));
  }

  free(args);
}

// [CRITICAL] performance
void fast_hash_join(arr_int first_fetch, vec_index first_selection,
                    arr_int second_fetch, vec_index second_selection,
                    JoinResult *res, thread_pool *pool) {
  if (first_selection.size > second_selection.size) {
    swap_arr_int(&first_fetch, &second_fetch);
    swap_vec_index(&first_selection, &second_selection);
    swap_vec_index_ptr(&res->first, &res->second);
  }

  if (first_selection.size < 1024) {
    hash_join(first_fetch, first_selection, second_fetch, second_selection,
              res);
    return;
  }

  size_t split = 1;
  jh_table *table[split];

  for (size_t i = 0; i < split; ++i) {
    // Populate the hash table
    fast_hash_params *first_params = malloc(sizeof(fast_hash_params));
    first_params->start = i * (first_selection.size / split);
    first_params->end =
        min((i + 1) * (first_selection.size / split), first_selection.size);
    first_params->select = &first_selection;
    first_params->fetch = &first_fetch;
    first_params->table = &table[i];
    add_task_thread_pool(pool, fast_hash_join_task, first_params);
  }

  wait_thread_pool(pool);

  // Perform join
  vec_jh_val first_vals;
  alloc_vec_jh_val(&first_vals, 10);
  for (size_t j = 0; j < second_selection.size; ++j) {
    int data = get_arr_int(&second_fetch, j);

    for (size_t i = 0; i < split; ++i) {
      jh_fast_get(table[i], data, &first_vals);
      for (size_t i = 0; i < first_vals.size; ++i) {
        fast_push_vec_index(res->first, get_vec_jh_val(&first_vals, i), 2);
        fast_push_vec_index(res->second, get_vec_index(&second_selection, j),
                            2);
      }
    }
  }
  free_vec_jh_val(&first_vals);

  for (size_t i = 0; i < split; ++i) {
    jh_deallocate(table[i]);
  }
}

// [CRITICAL] performance
void grace_hash_join(arr_int first_fetch, vec_index first_selection,
                     arr_int second_fetch, vec_index second_selection,
                     JoinResult *res) {
  if (first_selection.size > second_selection.size) {
    swap_arr_int(&first_fetch, &second_fetch);
    swap_vec_index(&first_selection, &second_selection);
    swap_vec_index_ptr(&res->first, &res->second);
  }

  cjh_table *second_table;
  cjh_allocate(&second_table, second_selection.size, true);

  // Populate first table
  for (size_t i = 0; i < second_selection.size; ++i) {
    cjh_put(second_table, get_arr_int(&second_fetch, i),
            get_vec_index(&second_selection, i));
  }

  cjh_table *first_table;
  // Don't scale to ensure same "bucket size"
  cjh_allocate(&first_table, second_table->capacity, false);

  // Populate second table
  for (size_t i = 0; i < first_selection.size; ++i) {
    cjh_put(first_table, get_arr_int(&first_fetch, i),
            get_vec_index(&first_selection, i));
  }

  // Make sure both tables have the same capacity
  assert(first_table->capacity == second_table->capacity);

  // Perform the join
  for (size_t i = 0; i < first_table->capacity; ++i) {
    // Check the second table first because it's less full
    if (second_table->entries[i] != NULL && first_table->entries[i] != NULL) {
      cjh_entry *first = first_table->entries[i];

      while (first != NULL) {
        cjh_entry *second = second_table->entries[i];

        while (second != NULL) {
          if (second->key == first->key) {
            fast_push_vec_index(res->first, first->value, 2);
            fast_push_vec_index(res->second, second->value, 2);
          }

          second = second->next;
        }

        first = first->next;
      }
    }
  }

  cjh_deallocate(first_table);
  cjh_deallocate(second_table);
}
