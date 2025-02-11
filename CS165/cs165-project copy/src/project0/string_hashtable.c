#include "string_hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Prime to be used in the polynomial rolling hash
#define P 31

// Simple implementation of a polynomial rolling hash function
key_hash string_hash(char* value) {
  key_hash result = 0;
  key_hash pow = 1;

  while (*value) {
    result += *value * pow;
    pow *= P;
    value++;
  }
  
  return result;
}

int strh_allocate(string_hashtable **ht, size_t capacity) {
  // Scale capacity appropriately
  int scaled_capacity = (double)capacity / STRH_MAX_LOAD_FACTOR;

  string_hashtable_entry *entries = malloc(scaled_capacity * sizeof(string_hashtable_entry));
  if (entries == NULL) {
    return -1;
  }

  // Initialize the entries
  for (int i = 0; i < scaled_capacity; ++i) {
    entries[i].psl = -1;
  }

  string_hashtable *table = malloc(sizeof(string_hashtable));
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

int strh_reallocate(string_hashtable *ht, size_t capacity) {
  string_hashtable_entry *old_entries = ht->entries;
  int old_capacity = ht->capacity;

  string_hashtable *new = NULL;
  strh_allocate(&new, capacity);

  for (int i = 0; i < old_capacity; ++i) {
    if (old_entries[i].psl != -1) {
      strh_put(new, old_entries[i].key, old_entries[i].value);
    }
  }

  free(old_entries);
  memcpy(ht, new, sizeof(string_hashtable));

  return 0;
}

static string_hashtable_entry swap_temp;
inline static void swap(string_hashtable_entry *first, string_hashtable_entry *second) {
  memcpy(&swap_temp, first, sizeof(string_hashtable_entry));
  memcpy(first, second, sizeof(string_hashtable_entry));
  memcpy(second, &swap_temp, sizeof(string_hashtable_entry));
}

int strh_put(string_hashtable *ht, char *key, void *value) {
  key_hash seed = string_hash(key);
  printf("  key is %s, with hash %lu\n", key, seed);

  string_hashtable_entry to_insert;
  to_insert.psl = 0;
  to_insert.key = key;
  to_insert.hash = seed;
  to_insert.value = value;

  string_hashtable_entry *current = &ht->entries[seed % ht->capacity];
  while (current->psl != -1) {
    // Check if entry already exists
    if (current->hash == to_insert.hash && !strcmp(current->key, to_insert.key)) {
      memcpy(current, &to_insert, sizeof(string_hashtable_entry));
      return 0;
    }

    if (to_insert.psl > current->psl) {
      swap(&to_insert, current);
    }

    ++to_insert.psl;
    current = &ht->entries[++seed % ht->capacity];
  }

  memcpy(current, &to_insert, sizeof(string_hashtable_entry));

  if (ht->size++ > ht->target_size) {
    if (strh_reallocate(ht, ht->capacity * 2) == -1) {
      return -1;
    }
  }

  return 0;
}

void *strh_get(string_hashtable *ht, char *key) {
  key_hash seed = string_hash(key);

  string_hashtable_entry current = ht->entries[seed % ht->capacity];
  while (current.psl != -1) {
    if (current.hash == seed && !strcmp(key, current.key)) {
      return current.value;
    }

    current = ht->entries[++seed % ht->capacity];
  }

  return NULL;
}

int strh_erase(string_hashtable *ht, char *key) {
  key_hash seed = string_hash(key);

  string_hashtable_entry *current = &ht->entries[seed % ht->capacity];
  string_hashtable_entry *prev = NULL;
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
      memcpy(prev, current, sizeof(string_hashtable_entry));
      current->psl = -1;
    }

    prev = current;
    current = &ht->entries[++seed % ht->capacity];
  }
  return 0;
}

int strh_deallocate(string_hashtable *ht) {
  free(ht->entries);
  free(ht);

  return 0;
}
