#pragma once

#include "client_context.h"
#include "db_types.h"
#include "message.h"
#include "operators.h"

typedef struct {
  vec_col *cols;
  bool first;
  size_t col_index;
  Column *current;
  int result;
  int negative;
} csv_load_data;

void partial_load(csv_load_data *data, char* line);
void initialize_indices(vec_col *cols);

void execute_operator(Operator *dbo, ClientContext *context, message *status);

void free_all_db();

int startup_db();
