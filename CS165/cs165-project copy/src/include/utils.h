#pragma once

#include <stdlib.h>

#define SWAP_IMPL(T, name)                                                     \
  static T swap_temp_##name;                                                   \
  inline static void swap_##name(T *first, T *second) {                        \
    swap_temp_##name = *first;                                                 \
    *first = *second;                                                          \
    *second = swap_temp_##name;                                                \
  }

static inline size_t min(size_t a, size_t b) { return (a < b) ? a : b; }
static inline size_t max(size_t a, size_t b) { return (a > b) ? a : b; }
