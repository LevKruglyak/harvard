#pragma once 

#include "vector.h"
#include "operators.h"
#include "types.h"
#include "db_types.h"
#include "variable.h"

typedef enum {
  // vec_int (could be column or fetch)
  COLUMN_NODE,
  // vec_index
  SELECT_NODE,
  // DataSingleton
  AGGREGATE_NODE,
} dep_node_type;

typedef union {
  vec_int *column;
  vec_index *select;
  DataSingleton *aggregate;
} dep_node_union;

typedef struct {
  dep_node_type type;
  dep_node_union data;
} dep_node; 

typedef enum {
  // column_node + filter -> select_node
  SELECT_LINK,
  // column_node + filter -> select_node
  RESELECT_LINK,
  // column_node + select_node -> column_node
  FETCH_LINK,
  // column_node + column_node -> column_node
  COMBINE_LINK,
  // column_node -> aggregate_node
  AGGREGATE_LINK,
} dep_link_type;

typedef struct {
  struct gen_var *col;
  Filter filter;
} select_link;

typedef struct {
  struct gen_var *col;
  struct gen_var *sel;
  Filter filter;
} reselect_link;

typedef struct {
  struct gen_var *col;
  struct gen_var *sel;
} fetch_link;

typedef struct {
  struct gen_var *first;
  struct gen_var *second;
  CombineType type;
} combine_link;

typedef struct {
  struct gen_var *col;
  AggregateType type;
} aggregate_link;

typedef union {
  select_link select;
  reselect_link reselect;
  fetch_link fetch;
  combine_link combine;
  aggregate_link aggregate;
} dep_link_union;

typedef struct {
  dep_link_type type;
  dep_link_union data;
  struct gen_var *out;
} dep_link;

typedef enum {
  GV_COLUMN,
  GV_VARIABLE,
} gen_var_type;

typedef struct {
  Variable *var;
  Column *col;
} gen_var_union;

typedef struct gen_var {
  gen_var_type type;
  gen_var_union data;
  dep_link *parent;
  int depth;
  // pthread_mutex_t lock;
  char _name[64];
} gen_var;

VECTOR_HEAD(gen_var *, gen_var)

typedef struct {
  vec_gen_var *variables;
} dep_graph;

