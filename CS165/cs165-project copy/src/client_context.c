#include "include/client_context.h"
#include "include/batch.h"
#include "include/globals.h"
#include "include/parse_utils.h"
#include "include/stats.h"
#include "include/thread_pool.h"
#include "include/variable.h"
#include <stdio.h>
#include <string.h>

void initialize_thread_pool(ClientContext *context) {
#ifdef ENABLE_MULTITHREADING
#ifdef DEFAULT_THREAD_COUNT 
  context->pool = new_thread_pool(DEFAULT_THREAD_COUNT);
  printf("using %d threads\n", DEFAULT_THREAD_COUNT);
#endif
#ifndef DEFAULT_THREAD_COUNT 
  context->pool = new_thread_pool(number_of_processors());
  printf("using %lu threads\n", number_of_processors());
#endif
#endif
}

ClientContext *new_client_context(int client_fd) {
  ClientContext *context = malloc(sizeof(ClientContext));
  alloc_strh(&context->variable_pool, 10);
  context->graph = NULL;
  context->client_fd = client_fd;
  context->pool = NULL;
  return context;
}

void delete_client_context(ClientContext *context) {
  FOR_STRH_START(context->variable_pool, var, Variable) { delete_var(var); }
  FOR_STRH_END
  free_strh(context->variable_pool);
  clear_batch(context);
  delete_thread_pool(context->pool);
  free(context);
}

void ensure_thread_pool(ClientContext *context) {
  if (context->pool == NULL) {
    initialize_thread_pool(context);
  }
}

void push_batch(ClientContext *context, Operator *oper) {
  if (!context->graph) {
    context->graph = create_dep_graph();
  }

  if (oper) {
    add_operator_to_dp(context->graph, context, oper);
  }
}

void execute_batch(ClientContext *context) {
  execute_dep_graph(context, context->graph, DEFAULT_BATCH_HEURISTIC);
}

void clear_batch(ClientContext *context) {
  if (context->graph != NULL) {
    destroy_dep_graph(context->graph);
    context->graph = NULL;
  }
}

void add_variable(ClientContext *context, Variable *var, char *name) {
  put_strh(context->variable_pool, name, var);
}

Variable *get_variable_force_type(ClientContext *context, VariableType type,
                                  char *name) {
  Variable *var = get_strh(context->variable_pool, name);

  if (var && var->type != type) {
    return NULL;
  }

  return var;
}

Variable *get_variable(ClientContext *context, char *name) {
  return get_strh(context->variable_pool, name);
}

Variable *create_variable(ClientContext *context, VariableType type,
                          char *name) {
  Variable *var = get_strh(context->variable_pool, name);

  if (var) {
    delete_var(var);
  }

  var = new_var(type);
  add_variable(context, var, name);

  return var;
}

Db *current_db;

Db *lookup_db(const char *name) {
  if (current_db && !strcmp(current_db->name, name))
    return current_db;
  return NULL;
}

Table *lookup_tbl(const char *name, Db *db) {
  return get_strh(db->tables, name);
}

Table *lookup_tbl_id(char *tbl_id) {
  char tbl_id_copy[MAX_TBL_ID];
  strcpy(tbl_id_copy, tbl_id);
  vec_str *args = tokenize(tbl_id_copy, ".");
  Table *tbl = NULL;
  if (args->size == 2) {
    Db *db = lookup_db(get_vec_str(args, 0));
    if (db) {
      tbl = lookup_tbl(get_vec_str(args, 1), db);
    }
  }
  delete_vec_str(args);
  return tbl;
}

Column *lookup_col(char *name, Table *tbl) {
  return get_strh(tbl->columns, name);
}

Column *lookup_col_id(char *col_id) {
  char col_id_copy[MAX_SIZE_HANDLE];
  strcpy(col_id_copy, col_id);
  vec_str *args = tokenize(col_id_copy, ".");
  Column *col = NULL;
  if (args->size == 3) {
    Db *db = lookup_db(get_vec_str(args, 0));
    if (db) {
      Table *tbl = lookup_tbl(get_vec_str(args, 1), db);
      if (tbl) {
        col = lookup_col(get_vec_str(args, 2), tbl);
      }
    }
  }
  delete_vec_str(args);
  return col;
}
