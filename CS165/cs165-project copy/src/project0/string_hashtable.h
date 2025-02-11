#ifndef CS165_STRING_HASHTABLE
#define CS165_STRING_HASHTABLE

#include <ctype.h>

#define STRH_MAX_LOAD_FACTOR 0.5;

typedef unsigned long key_hash;

typedef struct string_hashtable_entry {
  char* key;
  int psl;
  key_hash hash;
  void* value;
} string_hashtable_entry;

typedef struct string_hashtable {
  string_hashtable_entry *entries;
  int capacity;
  int target_size;
  int size;
} string_hashtable;

int strh_allocate(string_hashtable **ht, size_t capacity);
int strh_reallocate(string_hashtable *ht, size_t capacity);
int strh_put(string_hashtable *ht, char *key, void *value);
void *strh_get(string_hashtable *ht, char *key);
int strh_erase(string_hashtable *ht, char *key);
int strh_deallocate(string_hashtable *ht);

#endif
