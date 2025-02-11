#include "chained_hash_table.h"
#include "robinhood_hash_table.h"

typedef ch_hashtable ch_hashtable;
typedef rh_hashtable rh_hashtable;

#define allocate(ht, capacity)                                                 \
  _Generic(ht, ch_hashtable**: ch_allocate, rh_hashtable**: rh_allocate)(ht, capacity)
#define reallocate(ht, capacity)                                               \
  _Generic(ht, ch_hashtable *                                                  \
           : ch_reallocate, rh_hashtable *                                     \
           : rh_reallocate)(ht, capacity)
#define put(ht, key, value)                                                    \
  _Generic(ht, ch_hashtable * : ch_put, rh_hashtable * : rh_put)(ht, key, value)
#define get(ht, key, values, num_values, num_results)                          \
  _Generic(ht, ch_hashtable *                                                  \
           : ch_get, rh_hashtable *                                            \
           : rh_get)(ht, key, values, num_values, num_results)
#define erase(ht, key)                                                         \
  _Generic(ht, ch_hashtable * : ch_erase, rh_hashtable * : rh_erase)(ht, key)
#define deallocate(ht)                                                         \
  _Generic(ht, ch_hashtable *                                                  \
           : ch_deallocate, rh_hashtable *                                     \
           : rh_deallocate)(ht)
