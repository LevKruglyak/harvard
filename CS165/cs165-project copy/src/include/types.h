#pragma once

#include "vector.h"

#define OPTIONAL(T)                                                            \
  struct {                                                                     \
    union {                                                                    \
      T some;                                                                  \
      char none;                                                               \
    } data;                                                                    \
    char is_some;                                                              \
  }

#define OPTIONAL_IMPL(T, name)                                                 \
  typedef OPTIONAL(T) opt_##name;                                              \
                                                                               \
  static inline char is_some_##name(opt_##name opt) { return opt.is_some; }    \
  static inline char is_none_##name(opt_##name opt) {                          \
    return opt.is_some == 0;                                                   \
  }                                                                            \
  static inline T unwrap_##name(opt_##name opt) { return opt.data.some; }      \
                                                                               \
  static inline opt_##name some_##name(T data) {                               \
    opt_##name opt;                                                            \
    opt.is_some = 1;                                                           \
    opt.data.some = data;                                                      \
    return opt;                                                                \
  }                                                                            \
                                                                               \
  static inline opt_##name none_##name() {                                     \
    opt_##name opt;                                                            \
    opt.is_some = 0;                                                           \
    return opt;                                                                \
  }

// The primitive data types supported by this database system
typedef enum {
  INT,
  LONG,
  FLOAT,
} DataType;

typedef union {
  int _int;
  long long _long;
  double _double;
} DataSingletonUnion;

// Represents a singleton of a given data type
typedef struct {
  DataSingletonUnion data;
  DataType type;
} DataSingleton;

OPTIONAL_IMPL(int, int)
