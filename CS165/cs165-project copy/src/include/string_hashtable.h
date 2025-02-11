#pragma once

#include <stdlib.h>

#define STRH_MAX_LOAD_FACTOR 0.5
#define MAX_KEY_LENGTH 64 

typedef unsigned long str_hash;

str_hash hash_string(const char *str);

typedef unsigned long key_hash;

typedef struct string_hashtable_entry {
  char key[MAX_KEY_LENGTH];
  int psl;
  key_hash hash;
  void *value;
} string_hashtable_entry;

typedef struct string_hashtable {
  string_hashtable_entry *entries;
  size_t capacity;
  size_t target_size;
  size_t size;
} string_hashtable;

int alloc_strh(string_hashtable **ht, size_t capacity);
int realloc_strh(string_hashtable *ht, size_t capacity);
int put_strh(string_hashtable *ht, const char *key, void *value);
void *get_strh(string_hashtable *ht, const char *key);
int erase_strh(string_hashtable *ht, const char *key);
int free_strh(string_hashtable *ht);

#define FOR_STRH_START(strh, name, T)                                          \
  for (size_t i_##name = 0; i_##name < strh->capacity; ++i_##name) {           \
    string_hashtable_entry entry_##name = strh->entries[i_##name];             \
    T *name = (T *)entry_##name.value;                                         \
    if (entry_##name.psl != -1)

#define FOR_STRH_END }
