#ifndef CS165_CHAINED_HASH_TABLE
#define CS165_CHAINED_HASH_TABLE

#include "pool_alloc.h"
#include <sys/types.h>

#define CH_MAX_LOAD_FACTOR 0.9

typedef int keyType;
typedef int valType;

// A single entry in a hashtable
typedef struct ch_entry {
  keyType key;
  valType value;
  // Pointer to the next entry in the hash table
  struct ch_entry *next;
} ch_entry;

// The hashtable struct
typedef struct ch_hashtable {
  ch_entry **entries;
  pool_alloc *alloc;
  int capacity;
  int target_size;
  int size;
} ch_hashtable;

int ch_allocate(ch_hashtable **ht, int capacity);
int ch_reallocate(ch_hashtable *ht, int capacity);
int ch_put(ch_hashtable *ht, keyType key, valType value);
int ch_get(ch_hashtable *ht, keyType key, valType *values, int num_values,
           int *num_results);
int ch_erase(ch_hashtable *ht, keyType key);
int ch_deallocate(ch_hashtable *ht);

#endif
