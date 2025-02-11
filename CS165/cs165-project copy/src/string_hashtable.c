#include "include/string_hashtable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define P 31

str_hash hash_string(const char *str) {
  str_hash result = 0;
  str_hash pow = 1;

  while (*str) {
    result += *(str++) * pow;
    pow *= P;
  }

  return result;
}

int alloc_strh(string_hashtable **ht, size_t capacity) {
  // Scale capacity appropriately
  size_t scaled_capacity = (double)capacity / STRH_MAX_LOAD_FACTOR;

  string_hashtable_entry *entries =
      calloc(scaled_capacity, sizeof(string_hashtable_entry));
  if (entries == NULL) {
    return -1;
  }

  // Initialize the entries
  for (size_t i = 0; i < scaled_capacity; ++i) {
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

int realloc_strh(string_hashtable *ht, size_t capacity) {
  string_hashtable_entry *old_entries = ht->entries;
  size_t old_capacity = ht->capacity;

  string_hashtable *new = NULL;
  alloc_strh(&new, capacity);

  for (size_t i = 0; i < old_capacity; ++i) {
    if (old_entries[i].psl != -1) {
      put_strh(new, old_entries[i].key, old_entries[i].value);
    }
  }

  free(old_entries);
  memcpy(ht, new, sizeof(string_hashtable));
  free(new);

  return 0;
}

static string_hashtable_entry swap_temp;
inline static void swap(string_hashtable_entry *first,
                        string_hashtable_entry *second) {
  memcpy(&swap_temp, first, sizeof(string_hashtable_entry));
  memcpy(first, second, sizeof(string_hashtable_entry));
  memcpy(second, &swap_temp, sizeof(string_hashtable_entry));
}

int put_strh(string_hashtable *ht, const char *key, void *value) {
  assert(strlen(key) < MAX_KEY_LENGTH);

  string_hashtable_entry to_insert;
  to_insert.psl = 0;
  strcpy(to_insert.key, key);
  to_insert.hash = hash_string(to_insert.key);
  to_insert.value = value;

  key_hash seed = to_insert.hash;

  string_hashtable_entry *current = &ht->entries[seed % ht->capacity];
  while (current->psl != -1) {
    // Check if entry already exists
    if (current->hash == to_insert.hash &&
        !strcmp(current->key, to_insert.key)) {
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

  if (++ht->size > ht->target_size) {
    if (realloc_strh(ht, ht->capacity * 2) == -1) {
      return -1;
    }
  }

  return 0;
}

void *get_strh(string_hashtable *ht, const char *key) {
  key_hash seed = hash_string(key);
  size_t index = seed;

  string_hashtable_entry current = ht->entries[seed % ht->capacity];
  while (current.psl != -1) {
    // seed = hash_string(key);
    if (current.hash == seed && !strcmp(key, current.key)) {
      return current.value;
    }

    current = ht->entries[++index % ht->capacity];
  }

  return NULL;
}

int erase_strh(string_hashtable *ht, const char *key) {
  key_hash seed = hash_string(key);

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

int free_strh(string_hashtable *ht) {
  free(ht->entries);
  free(ht);

  return 0;
}
