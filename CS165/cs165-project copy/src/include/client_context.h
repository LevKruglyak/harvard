#pragma once

#include "db_types.h"
#include "dep_graph.h"
#include "string_hashtable.h"
#include "thread_pool.h"
#include "variable.h"
#include <stdbool.h>

typedef struct {
  int client_fd;
  string_hashtable *variable_pool;
  dep_graph *graph;
  thread_pool *pool;
} ClientContext;

void add_variable(ClientContext *context, Variable *var, char *name);
Variable *create_variable(ClientContext *context, VariableType type,
                          char *name);
Variable *get_variable_force_type(ClientContext *context, VariableType type,
                                  char *name);
Variable *get_variable(ClientContext *context, char *name);

static inline bool active_batch(ClientContext *context) {
  return context->graph != NULL;
}

void push_batch(ClientContext *context, Operator *oper);
void execute_batch(ClientContext *context);
void clear_batch(ClientContext *context);
void ensure_thread_pool(ClientContext *context);

ClientContext *new_client_context(int client_fd);
void delete_client_context(ClientContext *context);

extern Db *current_db;

Db *lookup_db(const char *name);
Table *lookup_tbl(const char *name, Db *db);
Table *lookup_tbl_id(char *tbl_id);
Column *lookup_col(char *name, Table *tbl);
Column *lookup_col_id(char *col_id);
