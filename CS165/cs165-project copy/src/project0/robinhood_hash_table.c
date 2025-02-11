#include "robinhood_hash_table.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rh_allocate(rh_hashtable **ht, int capacity) {
  // Scale capacity appropriately
  int scaled_capacity = (double)capacity / RH_MAX_LOAD_FACTOR;

  rh_entry *entries = malloc(scaled_capacity * sizeof(rh_entry));
  if (entries == NULL) {
    return -1;
  }

  // Initialize the entries
  for (int i = 0; i < scaled_capacity; ++i) {
    entries[i].psl = -1;
  }

  rh_hashtable *table = malloc(sizeof(rh_hashtable));
  if (table == NULL) {
    free(entries);
    return -1;
  }

  table->entries = entries;
  table->capacity = scaled_capacity;
  table->target_size = capacity;
  table->size = 0;
  *ht = table;

  return 0;
}

int rh_reallocate(rh_hashtable *ht, int capacity) {
  rh_entry *old_entries = ht->entries;
  int old_capacity = ht->capacity;

  rh_hashtable *new = NULL;
  rh_allocate(&new, capacity);

  for (int i = 0; i < old_capacity; ++i) {
    if (old_entries[i].psl != -1) {
      rh_put(new, old_entries[i].key, old_entries[i].value);
    }
  }

  free(old_entries);
  memcpy(ht, new, sizeof(rh_hashtable));

  return 0;
}

static rh_entry swap_temp;
inline static void swap(rh_entry *first, rh_entry *second) {
  memcpy(&swap_temp, first, sizeof(rh_entry));
  memcpy(first, second, sizeof(rh_entry));
  memcpy(second, &swap_temp, sizeof(rh_entry));
}

int rh_put(rh_hashtable *ht, keyType key, valType value) {
  unsigned int seed = simple_hash(key);
  unsigned int index = seed % ht->capacity;

  rh_entry to_insert;
  to_insert.psl = 0;
  to_insert.key = key;
  to_insert.value = value;

#define _MM_HINT_T0 1
  _mm_prefetch((const char *)&ht->entries[index], _MM_HINT_T0);

  rh_entry *current = &ht->entries[index];
  while (current->psl != -1) {
    if (to_insert.psl > current->psl) {
      swap(&to_insert, current);
    }

    ++to_insert.psl;
    ++seed;

    index = seed % ht->capacity;
    current = &ht->entries[index];
  }

  *current = to_insert;

  if (ht->size++ > ht->target_size) {
    if (rh_reallocate(ht, ht->capacity * 2) == -1) {
      return -1;
    }
  }

  return 0;
}

int rh_get(rh_hashtable *ht, keyType key, valType *values, int num_values,
           int *num_results) {
  unsigned int seed = simple_hash(key);
  unsigned int index = seed % ht->capacity;

  *num_results = 0;

#define _MM_HINT_T0 1
  _mm_prefetch((const char *)&ht->entries[index], _MM_HINT_T0);

  rh_entry current = ht->entries[index];
  while (current.psl != -1) {
    if (current.key == key) {
      if (*num_results < num_values) {
        values[*num_results++] = current.value;
      } else {
        break;
      }
    }

    ++seed;
    index = seed % ht->capacity;
    current = ht->entries[index];
  }

  return 0;
}

int rh_erase(rh_hashtable *ht, keyType key) {
  unsigned int seed = simple_hash(key);
  unsigned int index = seed % ht->capacity;

#define _MM_HINT_T0 1
  _mm_prefetch((const char *)&ht->entries[index], _MM_HINT_T0);

  rh_entry *current = &ht->entries[index];
  rh_entry *prev = NULL;
  while (current->psl != -1) {
    if (current->key == key) {
      // Remove entry
      --ht->size;
      current->psl = -1;
    }

    // Shift back if needed (TODO: keep shifting back until it moves to minimal
    // slot)
    if (prev && prev->psl == -1 && current->psl > 0) {
      --current->psl;
      *prev = *current;
      current->psl = -1;
    }

    prev = current;
    ++seed;
    index = seed % ht->capacity;
    current = &ht->entries[index];
  }
  return 0;
}

int rh_deallocate(rh_hashtable *ht) {
  free(ht->entries);
  free(ht);

  return 0;
}
