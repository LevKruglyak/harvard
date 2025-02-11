#pragma once

#include <stdbool.h>
#include <sys/time.h>
#include "db_types.h"
#include "types.h"

typedef enum {
  _SELECT,
  _FETCH,
  _AGGREGATE,
  _TIME,
} VariableType;

typedef struct {
  DataSingleton value;
} AggregateVariable;

typedef struct {
  struct timeval value; 
} TimeVariable;

typedef struct {
  unsigned index_low;
  unsigned index_high;
  Column *col;
} idx_select_variable;

typedef struct {
  vec_index selection;
  bool inverse;
  // Not the cleanest design
  bool no_block;

  // Also not the cleanest design (used for clustered indexes)
  idx_select_variable *idx_select;
} SelectVariable;

typedef struct {
  arr_int fetch;
  Column *col;
} FetchVariable;

typedef union {
  SelectVariable select;
  FetchVariable fetch;
  AggregateVariable aggregate;
  TimeVariable time;
} VariableUnion;

typedef struct {
  VariableType type;
  VariableUnion data;
} Variable;
 
Variable *new_var(VariableType type);
void delete_var(Variable *var);
void materialize_index_selection(SelectVariable *var);

VECTOR_HEAD(Variable *, var)
