#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VECTOR(T)                                                              \
  struct {                                                                     \
    T *data;                                                                   \
    size_t size;                                                               \
    size_t capacity;                                                           \
  }

#define ARRAY(T)                                                               \
  struct {                                                                     \
    T *data;                                                                   \
    size_t size;                                                               \
  }

#define ARRAY_HEAD(T, name)                                                    \
  typedef ARRAY(T) arr_##name;                                                 \
                                                                               \
  static inline void alloc_arr_##name(arr_##name *vec, size_t size) {          \
    vec->data = (T *)calloc(size, sizeof(T));                                  \
    vec->size = size;                                                          \
  }                                                                            \
                                                                               \
  static inline void new_arr_##name(size_t size) {                             \
    arr_##name *result = (arr_##name *)malloc(sizeof(arr_##name));             \
    alloc_arr_##name(result, size);                                            \
  }                                                                            \
                                                                               \
  static inline void free_arr_##name(arr_##name *vec) {                        \
    if (vec) {                                                                 \
      free(vec->data);                                                         \
    }                                                                          \
  }                                                                            \
                                                                               \
  static inline void delete_arr_##name(arr_##name *vec) {                      \
    free_arr_##name(vec);                                                      \
    free(vec);                                                                 \
  }                                                                            \
                                                                               \
  static inline T get_arr_##name(arr_##name *vec, size_t index) {              \
    return vec->data[index];                                                   \
  }                                                                            \
                                                                               \
  static inline void set_arr_##name(arr_##name *vec, T data, size_t index) {   \
    vec->data[index] = data;                                                   \
  }                                                                            \
                                                                               \
  static inline vec_##name to_vec_##name(arr_##name arr) {                     \
    vec_##name vec;                                                            \
    vec.data = arr.data;                                                       \
    vec.size = arr.size;                                                       \
    vec.capacity = arr.size;                                                   \
    return vec;                                                                \
  }

#define VECTOR_HEAD(T, name)                                                   \
  typedef VECTOR(T) vec_##name;                                                \
                                                                               \
  void alloc_vec_##name(vec_##name *vec, size_t capacity);                     \
  void append_vec_##name(vec_##name *dest, vec_##name *src);                   \
  void push_vec_##name(vec_##name *vec, T data);                               \
  void realloc_vec_##name(vec_##name *vec, size_t capacity);                   \
                                                                               \
  static inline T get_vec_##name(vec_##name *vec, size_t index) {              \
    assert(index < vec->capacity);                                             \
    return vec->data[index];                                                   \
  }                                                                            \
                                                                               \
  static inline vec_##name *new_vec_##name(size_t capacity) {                  \
    vec_##name *vec = (vec_##name *)malloc(sizeof(vec_##name));                \
    alloc_vec_##name(vec, capacity);                                           \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  static inline void free_vec_##name(vec_##name *vec) {                        \
    if (vec) {                                                                 \
      free(vec->data);                                                         \
    }                                                                          \
  }                                                                            \
                                                                               \
  static inline void delete_vec_##name(vec_##name *vec) {                      \
    free_vec_##name(vec);                                                      \
    free(vec);                                                                 \
  }                                                                            \
                                                                               \
  static inline void fast_push_vec_##name(vec_##name *vec, T data,             \
                                          size_t growth) {                     \
    if (vec->size >= vec->capacity - 1) {                                      \
      size_t new_capacity = vec->capacity * growth;                            \
      vec->data = (T *)realloc(vec->data, new_capacity * sizeof(T));           \
      assert(vec->data);                                                       \
      vec->capacity = new_capacity;                                            \
    }                                                                          \
                                                                               \
    vec->data[vec->size] = data;                                               \
    vec->size++;                                                               \
  }                                                                            \
                                                                               \
  static inline void set_vec_##name(vec_##name *vec, T data, size_t index) {   \
    vec->data[index] = data;                                                   \
  }

#define VECTOR_IMPL(T, name)                                                   \
  void alloc_vec_##name(vec_##name *vec, size_t capacity) {                    \
    if (vec) {                                                                 \
      vec->size = 0;                                                           \
      vec->capacity = capacity;                                                \
      vec->data = (T *)calloc(capacity, sizeof(T));                            \
      assert(vec->data);                                                       \
    }                                                                          \
  }                                                                            \
                                                                               \
  void realloc_vec_##name(vec_##name *vec, size_t capacity) {                  \
    if (capacity > vec->capacity) {                                            \
      vec->data = (T *)realloc(vec->data, capacity * sizeof(T));               \
      vec->capacity = capacity;                                                \
    }                                                                          \
  }                                                                            \
                                                                               \
  void push_vec_##name(vec_##name *vec, T data) {                              \
    if (vec->size >= vec->capacity - 1) {                                      \
      size_t new_capacity = vec->capacity * 2;                                 \
      vec->data = (T *)realloc(vec->data, new_capacity * sizeof(T));           \
      assert(vec->data);                                                       \
      vec->capacity = new_capacity;                                            \
    }                                                                          \
                                                                               \
    vec->data[vec->size] = data;                                               \
    vec->size++;                                                               \
  }                                                                            \
                                                                               \
  void append_vec_##name(vec_##name *dest, vec_##name *src) {                  \
    if (dest->size + src->size > dest->capacity) {                             \
      size_t new_capacity = (dest->size + src->size) * 2;                      \
      dest->data = (T *)realloc(dest->data, new_capacity * sizeof(T));         \
      assert(dest->data);                                                      \
      dest->capacity = new_capacity;                                           \
    }                                                                          \
                                                                               \
    memcpy(&dest->data[dest->size], src->data, src->size * sizeof(T));         \
    dest->size += src->size;                                                   \
  }

VECTOR_HEAD(int, int)
VECTOR_HEAD(char *, str)
VECTOR_HEAD(float, flt)
VECTOR_HEAD(void *, ptr)
VECTOR_HEAD(unsigned int, index)
VECTOR_HEAD(vec_index *, vec_index)
VECTOR_HEAD(vec_index, p_vec_index)

ARRAY_HEAD(int, int)
ARRAY_HEAD(char *, str)
ARRAY_HEAD(vec_index *, vec_index)
ARRAY_HEAD(vec_index, p_vec_index)
ARRAY_HEAD(float, flt)
