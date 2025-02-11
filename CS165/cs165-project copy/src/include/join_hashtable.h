#pragma once

#include "pool_alloc.h"
#include "vector.h"
#include <stdbool.h>
#include <sys/types.h>

// increase to reduce memory usage
#define RH_MAX_LOAD_FACTOR 0.2

typedef int jh_key;
typedef unsigned jh_val;

VECTOR_HEAD(jh_val, jh_val)

// A single entry in a hashtable
typedef struct {
  int psl;
  jh_key key;
  jh_val value;
} jh_entry;

// The hashtable struct
typedef struct {
  jh_entry *entries;
  size_t capacity;
  size_t target_size;
  size_t size;
} jh_table;

int jh_allocate(jh_table **ht, size_t capacity, bool scale);
int jh_reallocate(jh_table *ht, size_t capacity, bool scale);
int jh_put(jh_table *ht, jh_key key, jh_val value);
vec_jh_val jh_get(jh_table *ht, jh_key key);
void jh_fast_get(jh_table *ht, jh_key key, vec_jh_val *out);
int jh_erase(jh_table *ht, jh_key key);
int jh_deallocate(jh_table *ht);

// typedef struct {
//   jh_entry *entries;
//   vec_p
//   size_t capacity;
//   size_t target_size;
//   size_t size;
// } fast_jh_table;
//
// int fast_jh_allocate(fast_jh_table **ht, size_t capacity, bool scale);
// int fast_jh_reallocate(fast_jh_table *ht, size_t capacity, bool scale);
// int fast_jh_put(fast_jh_table *ht, jh_key key, jh_val value);
// vec_jh_val fast_jh_get(fast_jh_table *ht, jh_key key);
// void fast_jh_fast_get(fast_jh_table *ht, jh_key key, vec_jh_val *out);
// int fast_jh_erase(fast_jh_table *ht, jh_key key);
// int fast_jh_deallocate(fast_jh_table *ht);

#define CH_MAX_LOAD_FACTOR 0.9

typedef struct cjh_entry {
  jh_key key;
  jh_val value;
  struct cjh_entry *next;
} cjh_entry;

typedef struct cjh_table {
  cjh_entry **entries;
  pool_alloc *alloc;
  size_t capacity;
  size_t target_size;
  size_t size;
} cjh_table;

int cjh_allocate(cjh_table **ht, size_t capacity, bool scale);
int cjh_reallocate(cjh_table *ht, size_t capacity, bool scale);
int cjh_put(cjh_table *ht, jh_key, jh_val value);
vec_jh_val cjh_get(cjh_table *ht, jh_key key);
void cjh_fast_get(cjh_table *ht, jh_key key, vec_jh_val *out);
int cjh_erase(cjh_table *ht, jh_key key);
int cjh_deallocate(cjh_table *ht);
