#pragma once

#include "thread_pool.h"
#include "types.h"
#include "vector.h"

void select_range(vec_int data, vec_index *result, opt_int lower,
                  opt_int higher);

void reselect(vec_int data, vec_index original, vec_index *result,
              opt_int lower, opt_int higher);
void fetch(vec_int data, vec_index select, arr_int *fetch);
void fetch_index_range(vec_int data, unsigned low, unsigned high,
                       arr_int *fetch);

void reselect_inverse(vec_int data, vec_index original, size_t total,
                      vec_index *result, opt_int lower, opt_int higher);
void fetch_inverse(vec_int data, vec_index select, arr_int *fetch);

double avg(vec_int data);
int _min(vec_int data);
int _max(vec_int data);
long long sum(vec_int data);

void add(vec_int first, vec_int second, arr_int *result);
void sub(vec_int first, vec_int second, arr_int *result);

typedef struct {
  vec_index *first;
  vec_index *second;
} JoinResult;

void nested_loop_join(arr_int first_fetch, vec_index first_selection,
                      arr_int second_fetch, vec_index second_selection,
                      JoinResult *res);
void hash_join(arr_int first_fetch, vec_index first_selection,
               arr_int second_fetch, vec_index second_selection,
               JoinResult *res);

void fast_hash_join(arr_int first_fetch, vec_index first_selection,
               arr_int second_fetch, vec_index second_selection,
               JoinResult *res, thread_pool *pool);
void grace_hash_join(arr_int first_fetch, vec_index first_selection,
                     arr_int second_fetch, vec_index second_selection,
                     JoinResult *res);
