#include "include/log.h"
#include "include/message.h"
#include "include/operators.h"
#include "include/parse_utils.h"
#include "include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARSE_OPERATOR(T) Operator *parse_##T(vec_str *args, message *status)
#define PARSE_OPERATOR_FAMILY(T, ...)                                          \
  Operator *parse_##T(vec_str *args, message *status, __VA_ARGS__)

#define PARAM(N) get_vec_str(args, N)

#define NEW_OPERATOR(T)                                                        \
  (void)status;                                                                \
  Operator *dbo = malloc(sizeof(Operator));                                    \
  dbo->type = T

#define ENSURE_NUM_PARAMS(N)                                                   \
  if (args->size != (size_t)N) {                                               \
    ERROR_NULL("ERROR: incorrect number of arguments! `%d`\n", args->size);    \
  }

#define COPY_NAME_PARAMS(dest, N, max)                                         \
  char *name_##N = PARAM(N);                                                   \
  if (strlen(name_##N) >= max) {                                               \
    ERROR_NULL("ERROR: name is too long! `%s`\n", name_##N);                   \
  }                                                                            \
  strcpy(dest, name_##N);

#define ENSURE_MORE_THAN_NUM_PARAMS(N)                                         \
  if (args->size < (size_t)N) {                                                \
    ERROR_NULL("ERROR: incorrect number of arguments! `%d`\n", args->size);    \
  }

#define ENSURE_LESS_THAN_NUM_PARAMS(N)                                         \
  if (args->size > (size_t)N) {                                                \
    ERROR_NULL("ERROR: incorrect number of arguments! `%d`\n", args->size);    \
  }

PARSE_OPERATOR(print) {
  NEW_OPERATOR(PRINT);
  alloc_vec_str(&dbo->fields.print.params, args->size);

  for (size_t i = 0; i < args->size; ++i) {
    push_vec_str(&dbo->fields.print.params, get_vec_str(args, i));
  }

  return dbo;
}

PARSE_OPERATOR_FAMILY(aggregate, AggregateType type) {
  ENSURE_NUM_PARAMS(1);
  NEW_OPERATOR(AGGREGATE);
  COPY_NAME_PARAMS(dbo->fields.aggregate.var, 0, MAX_PARAM_SIZE);
  dbo->fields.aggregate.type = type;
  return dbo;
}

PARSE_OPERATOR_FAMILY(combine, CombineType type) {
  ENSURE_NUM_PARAMS(2);
  NEW_OPERATOR(COMBINE);
  COPY_NAME_PARAMS(dbo->fields.combine.first, 0, MAX_PARAM_SIZE);
  COPY_NAME_PARAMS(dbo->fields.combine.second, 1, MAX_PARAM_SIZE);
  dbo->fields.combine.type = type;
  return dbo;
}

#define PARSE_INT_PARAM_WITH_NULL(N)                                           \
  parse_int(PARAM(N));                                                         \
  char *string_##N = PARAM(N);                                                 \
  if (is_none_int(parse_int(string_##N)) && strcmp(string_##N, "null")) {      \
    ERROR_NULL("ERROR: incorrect number format `%s`\n", string_##N);           \
  }

PARSE_OPERATOR(select) {
  if (args->size == 3) {
    NEW_OPERATOR(SELECT);
    COPY_NAME_PARAMS(dbo->fields.select.col_id, 0, MAX_COL_ID);
    dbo->fields.select.filter.low = PARSE_INT_PARAM_WITH_NULL(1);
    dbo->fields.select.filter.high = PARSE_INT_PARAM_WITH_NULL(2);
    return dbo;
  } else if (args->size == 4) {
    NEW_OPERATOR(RESELECT);
    COPY_NAME_PARAMS(dbo->fields.reselect.select_var, 0, MAX_SIZE_HANDLE);
    COPY_NAME_PARAMS(dbo->fields.reselect.fetch_var, 1, MAX_PARAM_SIZE);
    dbo->fields.reselect.filter.low = PARSE_INT_PARAM_WITH_NULL(2);
    dbo->fields.reselect.filter.high = PARSE_INT_PARAM_WITH_NULL(3);
    return dbo;
  }

  ENSURE_NUM_PARAMS(-1);
  return NULL;
}

PARSE_OPERATOR(load) {
  ENSURE_NUM_PARAMS(1);
  NEW_OPERATOR(LOAD);
  dbo->fields.load.file = PARAM(0);
  return dbo;
}

PARSE_OPERATOR(fetch) {
  ENSURE_NUM_PARAMS(2)
  NEW_OPERATOR(FETCH);
  COPY_NAME_PARAMS(dbo->fields.fetch.col_id, 0, MAX_COL_ID);
  COPY_NAME_PARAMS(dbo->fields.fetch.select_var, 1, MAX_SIZE_HANDLE);
  return dbo;
}

PARSE_OPERATOR(create_tbl) {
  ENSURE_NUM_PARAMS(4);

  opt_int column_count = parse_int(PARAM(3));
  if (is_none_int(column_count)) {
    ERROR_NULL("ERROR: invalid number format `%s`!\n", PARAM(3));
  }

  NEW_OPERATOR(CREATE);
  dbo->fields.create.type = TABLE;
  strcpy(dbo->fields.create.fields.tbl.name, PARAM(1));
  strcpy(dbo->fields.create.fields.tbl.db_name, PARAM(2));
  dbo->fields.create.fields.tbl.columns_count = unwrap_int(column_count);
  return dbo;
}

PARSE_OPERATOR(create_col) {
  ENSURE_NUM_PARAMS(3);
  NEW_OPERATOR(CREATE);
  dbo->fields.create.type = COLUMN;
  COPY_NAME_PARAMS(dbo->fields.create.fields.column.name, 1, MAX_SIZE_NAME);
  COPY_NAME_PARAMS(dbo->fields.create.fields.column.tbl_id, 2, MAX_TBL_ID);
  return dbo;
}

PARSE_OPERATOR(create_db) {
  ENSURE_NUM_PARAMS(2);
  NEW_OPERATOR(CREATE);
  dbo->fields.create.type = DB;
  strcpy(dbo->fields.create.fields.db.name, PARAM(1));
  return dbo;
}

PARSE_OPERATOR(join) {
  ENSURE_NUM_PARAMS(5);
  NEW_OPERATOR(JOIN);
  strcpy(dbo->fields.join.first_fetch, PARAM(0));
  strcpy(dbo->fields.join.first_select, PARAM(1));
  strcpy(dbo->fields.join.second_fetch, PARAM(2));
  strcpy(dbo->fields.join.second_select, PARAM(3));

  if (strcmp(PARAM(4), "hash") == 0) {
    dbo->fields.join.type = HASH_JOIN;
  } else if (strcmp(PARAM(4), "nested-loop") == 0) {
    dbo->fields.join.type = NESTED_LOOP_JOIN;
  } else if (strcmp(PARAM(4), "grace-hash") == 0) {
    dbo->fields.join.type = GRACE_HASH_JOIN;
  } else {
    free(dbo);
    ERROR_NULL("ERROR: invalid join type `%s`!", PARAM(4));
  }

  return dbo;
}


PARSE_OPERATOR(create_idx) {
  ENSURE_NUM_PARAMS(4);

  index_form form;
  if (strcmp(PARAM(2), "btree") == 0) {
    form = BTREE;
  } else if (strcmp(PARAM(2), "sorted") == 0) {
    form = SORTED;
  } else {
    ERROR_NULL("ERROR: invalid index form `%s`\n", PARAM(2));
  }

  index_type type;
  if (strcmp(PARAM(3), "clustered") == 0) {
    type = CLUSTERED;
  } else if (strcmp(PARAM(3), "unclustered") == 0) {
    type = UNCLUSTERED;
  } else {
    ERROR_NULL("ERROR: invalid index type `%s`\n", PARAM(3));
  }

  NEW_OPERATOR(CREATE);
  dbo->fields.create.type = INDEX;
  strcpy(dbo->fields.create.fields.idx.col_id, PARAM(1));
  dbo->fields.create.fields.idx.type = type;
  dbo->fields.create.fields.idx.form = form;

  return dbo;
}

PARSE_OPERATOR(create) {
  ENSURE_MORE_THAN_NUM_PARAMS(1);
  char *create_type = PARAM(0);
  if (!strcmp(create_type, "db")) {
    return parse_create_db(args, status);
  } else if (!strcmp(create_type, "tbl")) {
    return parse_create_tbl(args, status);
  } else if (!strcmp(create_type, "col")) {
    return parse_create_col(args, status);
  } else if (!strcmp(create_type, "idx")) {
    return parse_create_idx(args, status);
  }

  ERROR_NULL("ERROR: unknown create type `%s`!\n", create_type);
}

PARSE_OPERATOR(insert) {
  ENSURE_MORE_THAN_NUM_PARAMS(2);

  vec_int values;
  alloc_vec_int(&values, 5);
  for (size_t i = 1; i < args->size; ++i) {
    opt_int value = parse_int(PARAM(i));
    if (is_none_int(value)) {
      free_vec_int(&values);
      ERROR_NULL("ERROR: invalid number format `%s`!\n", PARAM(i));
    }
    push_vec_int(&values, unwrap_int(value));
  }

  NEW_OPERATOR(INSERT);
  COPY_NAME_PARAMS(dbo->fields.insert.tbl_id, 0, MAX_TBL_ID);
  dbo->fields.insert.values = values;
  return dbo;
}

PARSE_OPERATOR(time) {
  ENSURE_LESS_THAN_NUM_PARAMS(2);

  NEW_OPERATOR(TIME);
  if (args->size == 0) {
    dbo->fields.time.since_var[0] = 0;
  } else if (args->size == 1) {
    COPY_NAME_PARAMS(dbo->fields.time.since_var, 0, MAX_SIZE_HANDLE);
  }
  return dbo;
}

vec_str *get_args(char **params, message *status) {
  size_t len = strlen(*params);
  if (strncmp(*params, "(", 1) == 0) {
    if (len < 1 || (*params)[len - 1] != ')') {
      ERROR_NULL("ERROR: missing closing `)`!\n");
    } else {
      (*params)[len - 1] = '\0';
      (*params)++;

      return tokenize(*params, ",");
    }
  } else {
    ERROR_NULL("ERROR: expected `(`!\n");
  }

  return NULL;
}

Operator *parse_shutdown() {
  Operator *dbo = malloc(sizeof(Operator));
  dbo->type = SHUTDOWN;
  return dbo;
}

Operator *parse_info() {
  Operator *dbo = malloc(sizeof(Operator));
  dbo->type = INFO;
  return dbo;
}

Operator *parse_batch_queries() {
  Operator *dbo = malloc(sizeof(Operator));
  dbo->type = BATCH_QUERY;
  return dbo;
}

Operator *parse_batch_execute() {
  Operator *dbo = malloc(sizeof(Operator));
  dbo->type = BATCH_EXECUTE;
  return dbo;
}

#define OPERATOR_COMPARE(T, name)                                              \
  if (strncmp(query, name, strlen(name)) == 0) {                               \
    query += strlen(name);                                                     \
    Operator *dbo = parse_##T();                                               \
    if (dbo) {                                                                 \
      strcpy(dbo->handle, handle);                                             \
    }                                                                          \
    return dbo;                                                                \
  }

#define OPERATOR_COMPARE_PARAMS(T, name)                                       \
  if (strncmp(query, name, strlen(name)) == 0) {                               \
    query += strlen(name);                                                     \
    vec_str *args = get_args(&query, status);                                  \
    if (args) {                                                                \
      Operator *dbo = parse_##T(args, status);                                 \
      delete_vec_str(args);                                                    \
      if (dbo) {                                                               \
        strcpy(dbo->handle, handle);                                           \
      }                                                                        \
      return dbo;                                                              \
    }                                                                          \
  }

#define OPERATOR_FAMILY_COMPARE_PARAMS(T, name, ...)                           \
  if (strncmp(query, name, strlen(name)) == 0) {                               \
    query += strlen(name);                                                     \
    vec_str *args = get_args(&query, status);                                  \
    if (args) {                                                                \
      Operator *dbo = parse_##T(args, status, __VA_ARGS__);                    \
      delete_vec_str(args);                                                    \
      if (dbo) {                                                               \
        strcpy(dbo->handle, handle);                                           \
      }                                                                        \
      return dbo;                                                              \
    }                                                                          \
  }

Operator *parse_command(char *query, message *status) {
  if (strncmp(query, "--", 2) == 0) {
    status->status = OK;
    return NULL;
  }

  // In case comment comes after command
  for (size_t i = 0; i < strlen(query) - 1; ++i) {
    if (query[i] == '-' && query[i + 1] == '-') {
      query[i] = 0;
      break;
    }
  }

  query = trim(query);

  char *equals_pointer = strchr(query, '=');
  char *handle = query;
  if (equals_pointer != NULL) {
    *equals_pointer = '\0';
    query = ++equals_pointer;

    if (strlen(handle) >= MAX_SIZE_HANDLE) {
      ERROR_NULL("ERROR: handle too long!\n");
    }
  } else {
    handle = "";
  }

  OPERATOR_COMPARE_PARAMS(create, "create");
  OPERATOR_COMPARE_PARAMS(load, "load");
  OPERATOR_COMPARE_PARAMS(select, "select");
  OPERATOR_COMPARE_PARAMS(fetch, "fetch");
  OPERATOR_COMPARE_PARAMS(print, "print");
  OPERATOR_COMPARE_PARAMS(time, "time");
  OPERATOR_COMPARE_PARAMS(join, "join");
  OPERATOR_FAMILY_COMPARE_PARAMS(aggregate, "avg", AVG);
  OPERATOR_FAMILY_COMPARE_PARAMS(aggregate, "min", MIN);
  OPERATOR_FAMILY_COMPARE_PARAMS(aggregate, "max", MAX);
  OPERATOR_FAMILY_COMPARE_PARAMS(aggregate, "sum", SUM);
  OPERATOR_FAMILY_COMPARE_PARAMS(combine, "add", ADD);
  OPERATOR_FAMILY_COMPARE_PARAMS(combine, "sub", SUB);
  OPERATOR_COMPARE_PARAMS(insert, "relational_insert");
  OPERATOR_COMPARE(shutdown, "shutdown");
  OPERATOR_COMPARE(info, "info");
  OPERATOR_COMPARE(batch_queries, "batch_queries");
  OPERATOR_COMPARE(batch_execute, "batch_execute");

  if (status->status == OK) {
    ERROR_NULL("ERROR: unknown command `%s`!\n", query);
  }

  return NULL;
}
