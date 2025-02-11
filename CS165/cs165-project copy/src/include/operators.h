#pragma once

#include "types.h"
#include "vector.h"
#include "sizes.h"
#include <stdlib.h>

typedef enum {
  CREATE,
  INSERT,
  LOAD,
  SHUTDOWN,
  SELECT,
  RESELECT,
  FETCH,
  AGGREGATE,
  COMBINE,
  INFO,
  PRINT,
  BATCH_QUERY,
  BATCH_EXECUTE,
  TIME,
  JOIN,
} OperatorType;

typedef enum { DB, TABLE, COLUMN, INDEX } CreateType;

typedef enum { AVG, MIN, MAX, SUM } AggregateType;

typedef enum { ADD, SUB } CombineType;

typedef struct {
  opt_int low;
  opt_int high;
} Filter;

/*
 *  OPERATOR DEFINITIONS
 */

typedef struct {
  char name[MAX_SIZE_NAME];
} DbCreateOperator;

typedef struct {
  char name[MAX_SIZE_NAME];
  char db_name[MAX_SIZE_NAME];
  size_t columns_count;
} TblCreateOperator;

typedef struct {
  char name[MAX_SIZE_NAME];
  char tbl_id[MAX_TBL_ID];
} ColumnCreateOperator;

typedef enum {
  SORTED,
  BTREE
} index_form;

typedef enum {
  CLUSTERED,
  UNCLUSTERED
} index_type;

typedef struct {
  char col_id[MAX_COL_ID];
  index_form form;
  index_type type;
} IndexCreateOperator;

typedef union {
  DbCreateOperator db;
  TblCreateOperator tbl;
  ColumnCreateOperator column;
  IndexCreateOperator idx;
} CreateOperatorUnion;

typedef struct {
  CreateType type;
  CreateOperatorUnion fields;
} CreateOperator;

typedef struct {
  char tbl_id[MAX_TBL_ID];
  vec_int values;
} InsertOperator;

typedef struct {
  char *file;
} LoadOperator;

typedef struct {
  char col_id[MAX_COL_ID];
  Filter filter;
} SelectOperator;

typedef struct {
  char select_var[MAX_SIZE_HANDLE];
  char fetch_var[MAX_PARAM_SIZE];
  Filter filter;
} ReSelectOperator;

typedef struct {
  char col_id[MAX_COL_ID];
  char select_var[MAX_SIZE_HANDLE];
} FetchOperator;

typedef struct {
  vec_str params;
} PrintOperator;

typedef struct {
  CombineType type;
  char first[MAX_PARAM_SIZE];
  char second[MAX_PARAM_SIZE];
} CombineOperator;

typedef struct {
  AggregateType type;
  char var[MAX_PARAM_SIZE];
} AggregateOperator;

typedef struct {
  char since_var[MAX_SIZE_HANDLE];
} TimeOperator;

typedef enum {
  HASH_JOIN,
  GRACE_HASH_JOIN,
  NESTED_LOOP_JOIN
} JoinType;

typedef struct {
  JoinType type;
  char first_select[MAX_SIZE_HANDLE];
  char second_select[MAX_SIZE_HANDLE];
  char first_fetch[MAX_SIZE_HANDLE];
  char second_fetch[MAX_SIZE_HANDLE];
} JoinOperator;

/*
 *  GENERIC OPERATOR
 */

typedef union {
  CreateOperator create;
  InsertOperator insert;
  LoadOperator load;
  SelectOperator select;
  ReSelectOperator reselect;
  PrintOperator print;
  FetchOperator fetch;
  AggregateOperator aggregate;
  CombineOperator combine;
  TimeOperator time;
  JoinOperator join;

  // Only here for macro purposes
  void *shutdown;
  void *info;
  void *batch_query;
  void *batch_execute;
} OperatorUnion;

typedef struct {
  OperatorType type;
  OperatorUnion fields;
  char handle[MAX_SIZE_HANDLE];
} Operator;

void free_operator(Operator *dbo);

VECTOR_HEAD(Operator *, oper)
