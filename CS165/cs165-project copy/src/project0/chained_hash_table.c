#include "chained_hash_table.h"
#include "pool_alloc.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

// Initialize the components of a hashtable.
// The size parameter is the expected number of elements to be inserted.
// This method returns an error code, 0 for success and -1 otherwise (e.g., if
// the parameter passed to the method is not null, if malloc fails, etc).
int ch_allocate(ch_hashtable **ht, int capacity) {
  // Premultiply by desired load factor
  int scaled_capacity = (double)capacity / CH_MAX_LOAD_FACTOR;

  ch_entry **entries = calloc(scaled_capacity, sizeof(ch_entry *));
  if (entries == NULL) {
    return -1;
  }

  int block_size = 4096 / sizeof(ch_entry);
  pool_alloc *alloc = new_pool(sizeof(ch_entry), block_size, scaled_capacity);
  if (alloc == NULL) {
    free(entries);
    return -1;
  }

  ch_hashtable *table = malloc(sizeof(ch_hashtable));
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

// Increase the expected number of elements in the hashtable
int ch_reallocate(ch_hashtable *ht, int capacity) {
  assert(ht->capacity <= capacity);

  ch_entry **entries = calloc(capacity, sizeof(ch_entry *));
  if (entries == NULL) {
    return -1;
  }

  // Fill up new table
  for (int i = 0; i < ht->capacity; ++i) {
    ch_entry *current = ht->entries[i];

    while (current != NULL) {
      ht->entries[i] = current->next;
      current->next = NULL;
      // printf("processing node %d %d\n", current->key, current->value);

      // Insert into new hashtable
      unsigned int index = simple_hash(current->key) % capacity;
      if (entries[index] == NULL) {
        entries[index] = current;
        // printf("inserted at root %d\n", index);
      } else {
        ch_entry *insert_current = entries[index];
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

// This method inserts a key-value pair into the hash table.
// It returns an error code, 0 for success and -1 otherwise (e.g., if malloc is
// called and fails).
int ch_put(ch_hashtable *ht, keyType key, valType value) {
  ch_entry *entry = (ch_entry *)malloc_chunk(ht->alloc);
  if (entry == NULL) {
    return -1;
  }

  entry->key = key;
  entry->value = value;
  entry->next = NULL;

  unsigned int index = simple_hash(key) % ht->capacity;

  // Attempt to insert the entry into the table
  if (ht->entries[index] == NULL) {
    ht->entries[index] = entry;
  } else {
    ch_entry *current = ht->entries[index];
    while (current->next != NULL) {
      current = current->next;
    }

    current->next = entry;
  }

  if (ht->size++ > ht->target_size) {
    if (ch_reallocate(ht, ht->capacity * 2) == -1) {
      return -1;
    }
  }

  return 0;
}

// This method retrieves entries with a matching key and stores the
// corresponding values in the values array. The size of the values array is
// given by the parameter num_values. If there are more matching entries than
// num_values, they are not stored in the values array to avoid a buffer
// overflow. The function returns the number of matching entries using the
// num_results pointer. If the value of num_results is greater than num_values,
// the caller can invoke this function again (with a larger buffer) to get
// values that it missed during the first call. This method returns an error
// code, 0 for success and -1 otherwise (e.g., if the hashtable is not
// allocated).
int ch_get(ch_hashtable *ht, keyType key, valType *values, int num_values,
           int *num_results) {
  if (ht == NULL) {
    return -1;
  }
  unsigned int index = simple_hash(key) % ht->capacity;

  // Loop through the linked list
  ch_entry *current = ht->entries[index];
  *num_results = 0;
  while (current != NULL && *num_results < num_values) {
    if (current->key == key) {
      values[*num_results] = current->value;
      (*num_results)++;
    }

    current = current->next;
  }

  return 0;
}

// This method erases all key-value pairs with a given key from the hash table.
// It returns an error code, 0 for success and -1 otherwise (e.g., if the
// hashtable is not allocated).
int ch_erase(ch_hashtable *ht, keyType key) {
  if (ht == NULL) {
    return -1;
  }

  unsigned int index = simple_hash(key) % ht->capacity;

  // Loop through the linked list
  ch_entry *previous = NULL;
  ch_entry *current = ht->entries[index];

  // Start case
  while (current != NULL) {
    if (current->key == key) {
      if (previous == NULL) {
        ht->entries[index] = current->next;
      } else {
        previous->next = current->next;
      }

      ch_entry *temp = current->next;
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

// This method frees all memory occupied by the hash table.
// It returns an error code, 0 for success and -1 otherwise.
int ch_deallocate(ch_hashtable *ht) {
  if (ht == NULL) {
    return 0;
  }

  // Loop through the entire hash table
  ch_entry *current = NULL;
  ch_entry *temp = NULL;

  for (int index = 0; index < ht->capacity; ++index) {
    current = ht->entries[index];

    while (current != NULL) {
      temp = current->next;
      free_chunk(ht->alloc, current);
      current = temp;
    }
  }

  delete_pool(ht->alloc);
  free(ht->entries);
  free(ht);

  return 0;
}
