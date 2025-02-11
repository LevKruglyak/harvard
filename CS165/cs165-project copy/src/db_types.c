#include "include/db_types.h"
#include "include/string_hashtable.h"
#include <string.h>

VECTOR_IMPL(Column *, col)

Db *new_db(char *name) {
  Db *db = malloc(sizeof(Db));
  strcpy(db->name, name);
  alloc_strh(&db->tables, 10);
  return db;
}

Table *new_table(char *name, size_t columns_count) {
  Table *tbl = malloc(sizeof(Table));
  strcpy(tbl->name, name);
  tbl->columns_count = columns_count;
  alloc_strh(&tbl->columns, 10);
  tbl->ordered_columns = new_vec_col(10);
  tbl->primary = NULL;
  return tbl;
}

Column *new_column(char *name) {
  Column *col = malloc(sizeof(Column));
  strcpy(col->name, name);
  col->idx = NULL;
  alloc_vec_int(&col->data, INITIAL_COL_CAPACITY);
  assert(col->data.data);
  return col;
}

void delete_column(Column *col) {
  if (col) {
    free_vec_int(&col->data);
    if (col->idx) {
      destroy_index(col->idx);
    }
  }
  free(col);
}

void delete_table(Table *tbl) {
  if (tbl) {
    FOR_STRH_START(tbl->columns, col, Column) { delete_column(col); }
    FOR_STRH_END
    free_strh(tbl->columns);
    delete_vec_col(tbl->ordered_columns);
  }
  free(tbl);
}

void delete_db(Db *db) {
  if (db) {
    FOR_STRH_START(db->tables, tbl, Table) { delete_table(tbl); }
    FOR_STRH_END
    free_strh(db->tables);
  }
  free(db);
}
