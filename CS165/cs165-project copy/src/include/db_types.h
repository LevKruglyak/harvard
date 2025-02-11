#pragma once

#include "btree.h"
#include "operators.h"
#include "string_hashtable.h"
#include "vector.h"
#include "sizes.h"

#define DEFAULT_VEC_SIZE 5
#define INITIAL_COL_CAPACITY 1024

typedef struct col_t {
  char name[MAX_SIZE_NAME];
  unsigned col_id;
  vec_int data;
  struct table_t *tbl;
  struct idx_t *idx;
} Column;

VECTOR_HEAD(Column *, col)

typedef struct table_t {
  char name[MAX_SIZE_NAME];
  size_t columns_count;
  string_hashtable *columns;
  vec_col *ordered_columns;
  // Pointer to the column containing the primary clustered index
  Column *primary;
} Table;

typedef struct {
  char name[MAX_SIZE_NAME];
  string_hashtable *tables;
} Db;

Db *new_db(char *name);
Table *new_table(char *name, size_t columns_count);
Column *new_column(char *name);

void delete_db(Db *db);
void delete_table(Table *tbl);
void delete_column(Column *col);

typedef union {
  vec_kp sorted;
  btree *btree;
} index_form_union;

typedef struct {
  Column *col;
  vec_int data;
} col_copy;

VECTOR_HEAD(col_copy, col_copy)
ARRAY_HEAD(col_copy, col_copy)

typedef struct {
  arr_col_copy copies;
} clustered_index_data;

typedef struct idx_t {
  index_type type;
  clustered_index_data cluster_data;
  index_form form;
  index_form_union form_data;
  int histogram[100];
  bool initialized;
} idx;

idx *create_index(index_form form, index_type type, Column *col);
void initialize_index(idx *index, Column *col);
void destroy_index(idx *index);
