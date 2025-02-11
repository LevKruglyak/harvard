#ifndef CS165_ROBINHOOD_HASH_TABLE
#define CS165_ROBINHOOD_HASH_TABLE

#include <sys/types.h>

#define RH_MAX_LOAD_FACTOR 0.5

typedef int keyType;
typedef int valType;

// A single entry in a hashtable
typedef struct rh_entry {
  int psl;
  keyType key;
  valType value;
} rh_entry;

// The hashtable struct
typedef struct rh_hashtable {
  rh_entry *entries;
  int capacity;
  int target_size;
  int size;
} rh_hashtable;

int rh_allocate(rh_hashtable **ht, int capacity);
int rh_reallocate(rh_hashtable *ht, int capacity);
int rh_put(rh_hashtable *ht, keyType key, valType value);
int rh_get(rh_hashtable *ht, keyType key, valType *values, int num_values,
           int *num_results);
int rh_erase(rh_hashtable *ht, keyType key);
int rh_deallocate(rh_hashtable *ht);

#endif
