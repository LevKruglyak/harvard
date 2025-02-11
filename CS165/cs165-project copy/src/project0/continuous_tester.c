#include "hash_table.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int hash_table_size = 4096;
  int key_numbers = 10;
  int num_samples = 10000000;

  rh_hashtable *ht = NULL;
  assert(allocate(&ht, hash_table_size) == 0);

  int seed = 2;
  srand(seed);

  int entries[key_numbers];
  for (int j = 0; j < key_numbers; ++j) {
    entries[j] = 0;
  }

  int key = 0;
  for (int j = 0; j < num_samples; ++j) {
    key = (key * 1275 + 10) % key_numbers;
    // Decide whether to insert or delete
    if (key % 113 > 60) {
      // Insert
      for (int i = 0; i < (key % 3); ++i) {
        put(ht, key, 0);
        // printf("put %d\n", key);
        entries[key]++;
      }
    } else {
      // Delete
      erase(ht, key);
      // printf("erase %d\n", key);
      entries[key] = 0;
    }

    // Run check
    int size = 0;
    for (int i = 0; i < key_numbers; ++i) {
      size += entries[i];
    }
    // printf("%d == %d\n", size, ht->size);
    assert(size == ht->size);

    int num_results;
    int results[num_results];
    for (int i = 0; i < key_numbers; ++i) {
      get(ht, i, results, num_samples, &num_results);
      if (num_results != entries[i]) {
        // printf("failed %d -> %d != %d\n", i, num_results, entries[i]);
        return -1;
      }
    }
  }

  assert(deallocate(ht) == 0);
}
