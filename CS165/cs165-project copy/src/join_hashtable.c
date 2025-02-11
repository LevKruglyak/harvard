#include "include/join_hashtable.h"
#include "include/utils.h"
#include "include/vector.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VECTOR_IMPL(jh_val, jh_val)

unsigned hash_int(unsigned x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  return (x >> 16) ^ x;
}

int jh_allocate(jh_table **ht, size_t capacity, bool scale) {
  // Scale capacity appropriately
  int scaled_capacity =
      scale ? (double)capacity / RH_MAX_LOAD_FACTOR : capacity;

  jh_entry *entries = malloc(scaled_capacity * sizeof(jh_entry));
  if (entries == NULL) {
    return -1;
  }

  // Initialize the entries
  for (int i = 0; i < scaled_capacity; ++i) {
    entries[i].psl = -1;
  }

  jh_table *table = malloc(sizeof(jh_table));
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

int jh_reallocate(jh_table *ht, size_t capacity, bool scale) {
  jh_entry *old_entries = ht->entries;
  int old_capacity = ht->capacity;

  jh_table *new = NULL;
  jh_allocate(&new, capacity, scale);

  for (int i = 0; i < old_capacity; ++i) {
    if (old_entries[i].psl != -1) {
      jh_put(new, old_entries[i].key, old_entries[i].value);
    }
  }

  free(old_entries);
  memcpy(ht, new, sizeof(jh_table));

  return 0;
}

SWAP_IMPL(jh_entry, jh_entry)

int jh_put(jh_table *ht, jh_key key, jh_val value) {
  unsigned int seed = hash_int(key);

  jh_entry to_insert;
  to_insert.psl = 0;
  to_insert.key = key;
  to_insert.value = value;

  jh_entry *current = &ht->entries[seed % ht->capacity];
  while (current->psl != -1) {
    if (to_insert.psl > current->psl) {
      swap_jh_entry(&to_insert, current);
    }

    ++to_insert.psl;
    current = &ht->entries[++seed % ht->capacity];
  }

  memcpy(current, &to_insert, sizeof(jh_entry));

  if (ht->size++ > ht->target_size) {
    if (jh_reallocate(ht, ht->capacity * 2, true) == -1) {
      return -1;
    }
  }

  return 0;
}

vec_jh_val jh_get(jh_table *ht, jh_key key) {
  vec_jh_val res;
  alloc_vec_jh_val(&res, 2);

  jh_fast_get(ht, key, &res);

  return res;
}

void jh_fast_get(jh_table *ht, jh_key key, vec_jh_val *out) {
  // Clear
  out->size = 0;
  unsigned int seed = hash_int(key);

  jh_entry current = ht->entries[seed % ht->capacity];
  while (current.psl != -1) {
    if (current.key == key) {
      push_vec_jh_val(out, current.value);
    }

    current = ht->entries[++seed % ht->capacity];
  }
}

int jh_erase(jh_table *ht, jh_key key) {
  unsigned int seed = hash_int(key);

  jh_entry *current = &ht->entries[seed % ht->capacity];
  jh_entry *prev = NULL;
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
      memcpy(prev, current, sizeof(jh_entry));
      current->psl = -1;
    }

    prev = current;
    current = &ht->entries[++seed % ht->capacity];
  }
  return 0;
}

int jh_deallocate(jh_table *ht) {
  free(ht->entries);
  free(ht);

  return 0;
}

int cjh_allocate(cjh_table **ht, size_t capacity, bool scale) {
  int scaled_capacity =
      scale ? (double)capacity / RH_MAX_LOAD_FACTOR : capacity;

  cjh_entry **entries = calloc(scaled_capacity, sizeof(cjh_entry *));
  if (entries == NULL) {
    return -1;
  }

  int block_size = 4096 / sizeof(cjh_entry);
  pool_alloc *alloc =
      new_pool_alloc(sizeof(cjh_entry), block_size, scaled_capacity);
  if (alloc == NULL) {
    free(entries);
    return -1;
  }

  cjh_table *table = malloc(sizeof(cjh_table));
  if (table == NULL) {
    free(entries);
    free(alloc);
    return -1;
  }

  table->capacity = scaled_capacity;
  table->size = 0;
  table->target_size = capacity;
  table->entries = entries;
  table->alloc = alloc;
  *ht = table;

  return 0;
}

int cjh_reallocate(cjh_table *ht, size_t capacity, bool scale) {
  (void)scale;
  assert(ht->capacity <= capacity);

  cjh_entry **entries = calloc(capacity, sizeof(cjh_entry *));
  if (entries == NULL) {
    return -1;
  }

  // Fill up new table
  for (size_t i = 0; i < ht->capacity; ++i) {
    cjh_entry *current = ht->entries[i];

    while (current != NULL) {
      ht->entries[i] = current->next;
      current->next = NULL;
      // printf("processing node %d %d\n", current->key, current->value);

      // Insert into new hashtable
      unsigned int index = hash_int(current->key) % capacity;
      if (entries[index] == NULL) {
        entries[index] = current;
        // printf("inserted at root %d\n", index);
      } else {
        cjh_entry *insert_current = entries[index];
        while (insert_current->next != NULL) {
          insert_current = insert_current->next;
        }

        insert_current->next = current;
      }

      current = ht->entries[i];
    }
  }

  // Update table information
  free(ht->entries);
  ht->entries = entries;
  ht->capacity = capacity;
  ht->target_size = capacity / CH_MAX_LOAD_FACTOR + 1;

  return 0;
}

int cjh_put(cjh_table *ht, jh_key key, jh_val value) {
  cjh_entry *entry = (cjh_entry *)malloc_chunk(ht->alloc);
  if (entry == NULL) {
    return -1;
  }

  entry->key = key;
  entry->value = value;
  entry->next = NULL;

  unsigned int index = hash_int(key) % ht->capacity;

  // Attempt to insert the entry into the table
  if (ht->entries[index] == NULL) {
    ht->entries[index] = entry;
  } else {
    cjh_entry *current = ht->entries[index];
    while (current->next != NULL) {
      current = current->next;
    }

    current->next = entry;
  }

  if (ht->size++ > ht->target_size) {
    if (cjh_reallocate(ht, ht->capacity * 2, false) == -1) {
      return -1;
    }
  }

  return 0;
}

vec_jh_val cjh_get(cjh_table *ht, jh_key key) {
  vec_jh_val result;
  alloc_vec_jh_val(&result, 2);

  cjh_fast_get(ht, key, &result);

  return result;
}

void cjh_fast_get(cjh_table *ht, jh_key key, vec_jh_val *out) {
  // Clear
  out->size = 0;

  unsigned int index = hash_int(key) % ht->capacity;

  // Loop through the linked list
  cjh_entry *current = ht->entries[index];
  while (current != NULL) {
    if (current->key == key) {
      push_vec_jh_val(out, current->value);
    }

    current = current->next;
  }
}

int cjh_erase(cjh_table *ht, jh_key key) {
  if (ht == NULL) {
    return -1;
  }

  unsigned int index = hash_int(key) % ht->capacity;

  // Loop through the linked list
  cjh_entry *previous = NULL;
  cjh_entry *current = ht->entries[index];

  // Start case
  while (current != NULL) {
    if (current->key == key) {
      if (previous == NULL) {
        ht->entries[index] = current->next;
      } else {
        previous->next = current->next;
      }

      cjh_entry *temp = current->next;
      free_chunk(ht->alloc, current);
      --ht->size;
      current = temp;
    } else {
      previous = current;
      current = current->next;
    }
  }
  return 0;
}

int cjh_deallocate(cjh_table *ht) {
  if (ht == NULL) {
    return 0;
  }

  // Loop through the entire hash table
  cjh_entry *current = NULL;
  cjh_entry *temp = NULL;

  for (size_t index = 0; index < ht->capacity; ++index) {
    current = ht->entries[index];

    while (current != NULL) {
      temp = current->next;
      free_chunk(ht->alloc, current);
      current = temp;
    }
  }

  delete_pool_alloc(ht->alloc);
  free(ht->entries);
  free(ht);

  return 0;
}
