#include "include/batch.h"
#include "include/client_context.h"
#include "include/db_api.h"
#include "include/db_internals.h"
#include "include/db_types.h"
#include "include/globals.h"
#include "include/message.h"
#include "include/operators.h"
#include "include/parse_utils.h"
#include "include/rand.h"
#include "include/string_hashtable.h"
#include "include/utils.h"
#include "include/variable.h"
#include "include/vector.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define OPERATOR_EXECUTE_DEFINITION(name, type)                                \
  void db_##name(type dbo, char *handle, ClientContext *context,               \
                 message *status)

#define OPERATOR_EXECUTE(T, name)                                              \
  case T:                                                                      \
    db_##name(dbo->fields.name, dbo->handle, context, status);                 \
    break

#define SUBOPERATOR_EXECUTE(T, pre, name)                                      \
  case T:                                                                      \
    db_##pre##_##name(dbo.fields.name, handle, context, status);               \
    break

#define ENSURE_NAME_FITS(name, max)                                            \
  if (strlen(name) > max) {                                                    \
    ERROR("ERROR: name `%s` is too long!\n", name);                            \
  }

#define ENSURE_DB_EXISTS(name, db_name)                                        \
  Db *db_name = lookup_db(name);                                               \
  if (!db_name) {                                                              \
    ERROR("ERROR: db `%s` does not exist!\n", name);                           \
  }

#define ENSURE_TBL_EXISTS(tbl_id, tbl_name)                                    \
  Table *tbl_name = lookup_tbl_id(tbl_id);                                     \
  if (!tbl_name) {                                                             \
    ERROR("ERROR: invalid tbl_id `%s`!\n", tbl_id);                            \
  }

#define ENSURE_COL_EXISTS(col_id, col_name)                                    \
  Column *col_name = lookup_col_id(col_id);                                    \
  if (!col_name) {                                                             \
    ERROR("ERROR: invalid col_id `%s`!\n", col_id);                            \
  }

#define ENSURE_TYPED_VAR_EXISTS(var, type, var_name, context)                  \
  Variable *var_name = get_variable_force_type(context, type, var);            \
  if (!var_name) {                                                             \
    ERROR("ERROR: no variable `%s` of required type!\n", var);                 \
  }

#define ENSURE_FETCH_VAR_OR_COL_EXISTS(var, vec_int_name, context)             \
  size_t size_##vec_int_name;                                                  \
  (void)size_##vec_int_name;                                                   \
  vec_int vec_int_name;                                                        \
  Variable *var_##vec_int_name =                                               \
      get_variable_force_type(context, _FETCH, var);                           \
  if (!var_##vec_int_name) {                                                   \
    Column *col = lookup_col_id(var);                                          \
    if (!col) {                                                                \
      ERROR("ERROR: no column or variable `%s` of required type!\n", var);     \
    }                                                                          \
    vec_int_name = col->data;                                                  \
    size_##vec_int_name = col->data.size;                                      \
  } else {                                                                     \
    vec_int_name = to_vec_int(var_##vec_int_name->data.fetch.fetch);           \
    size_##vec_int_name = var_##vec_int_name->data.fetch.col->data.size;       \
  }

#define ENSURE_VAR_EXISTS(var, var_name, context)                              \
  Variable *var_name = get_variable(context, var);                             \
  if (!var_name) {                                                             \
    ERROR("ERROR: no variable `%s` of required type!\n", var);                 \
  }

OPERATOR_EXECUTE_DEFINITION(create_db, DbCreateOperator) {
  (void)context;
  (void)handle;
  // Overwrite db if one already exists
  if (current_db) {
    delete_db(current_db);
  }

  ENSURE_NAME_FITS(dbo.name, MAX_SIZE_NAME);
  current_db = new_db(dbo.name);
}

OPERATOR_EXECUTE_DEFINITION(create_tbl, TblCreateOperator) {
  (void)context;
  (void)handle;

  ENSURE_NAME_FITS(dbo.name, MAX_SIZE_NAME);
  ENSURE_DB_EXISTS(dbo.db_name, db);

  if (lookup_tbl(dbo.name, db) != NULL) {
    ERROR("ERROR: tbl `%s` already exists!\n", dbo.name);
  }

  Table *tbl = new_table(dbo.name, dbo.columns_count);
  put_strh(db->tables, dbo.name, tbl);
}

OPERATOR_EXECUTE_DEFINITION(create_column, ColumnCreateOperator) {
  (void)context;
  (void)handle;

  ENSURE_NAME_FITS(dbo.name, MAX_SIZE_NAME);
  ENSURE_TBL_EXISTS(dbo.tbl_id, tbl);

  if (lookup_col(dbo.name, tbl) != NULL) {
    ERROR("ERROR: col `%s` already exists!\n", dbo.name);
  }

  if (tbl->columns->size == tbl->columns_count) {
    ERROR("ERROR: too many columns!\n");
  }

  Column *col = new_column(dbo.name);
  col->tbl = tbl;
  col->col_id = tbl->ordered_columns->size;
  col->idx = NULL;
  put_strh(tbl->columns, dbo.name, col);
  push_vec_col(tbl->ordered_columns, col);
}

OPERATOR_EXECUTE_DEFINITION(create_idx, IndexCreateOperator) {
  (void)context;
  (void)handle;

  ENSURE_COL_EXISTS(dbo.col_id, col);
  // col->idx = create_index(dbo.form, dbo.type, col);
  //
  // if (dbo.type == CLUSTERED && col->tbl->primary == NULL) {
  //   col->tbl->primary = col;
  // }
}

OPERATOR_EXECUTE_DEFINITION(create, CreateOperator) {
  (void)context;
  (void)handle;

  switch (dbo.type) {
    SUBOPERATOR_EXECUTE(DB, create, db);
    SUBOPERATOR_EXECUTE(TABLE, create, tbl);
    SUBOPERATOR_EXECUTE(COLUMN, create, column);
    SUBOPERATOR_EXECUTE(INDEX, create, idx);
  }
}

OPERATOR_EXECUTE_DEFINITION(select, SelectOperator) {
  (void)context;
  (void)handle;

  ENSURE_COL_EXISTS(dbo.col_id, col);

  if (handle[0]) {
    Variable *selection = create_variable(context, _SELECT, handle);
    SelectVariable *select_var = &selection->data.select;
    vec_index *select_var_index = &select_var->selection;

    if (col->idx != NULL) {
      printf("deciding if we should use index...\n");
      int low =
          is_some_int(dbo.filter.low) ? unwrap_int(dbo.filter.low) : -INT_MAX;
      int high =
          is_some_int(dbo.filter.high) ? unwrap_int(dbo.filter.high) : INT_MAX;

      // Estimate selectivity
      size_t num_samples = col->data.size * sizeof(int) / (128 * 4096);
      if (num_samples > MAX_SELECTIVITY_ESTIMATOR_SAMPLES) {
        num_samples = MAX_SELECTIVITY_ESTIMATOR_SAMPLES;
      }

      if (num_samples < 1) {
        num_samples = 1;
      }

      float selectivity = 0.0;
      mt_rand rand = m_srand(time(NULL));
      for (size_t i = 0; i < num_samples; ++i) {
        size_t page_index = m_gen_rand(&rand) % col->data.size;
        int *page = &col->data.data[page_index];
        size_t page_size = min(SELECTIVITY_ESTIMATOR_PAGE_SIZE / sizeof(int),
                               col->data.size - page_index);

        float _selectivity = 0.0;
        for (size_t k = 0; k < page_size; ++k) {
          int data = page[k];
          if ((low <= data) & (high > data)) {
            _selectivity++;
          }
        }
        selectivity += _selectivity / (float)page_size;
      }
      selectivity /= (float)num_samples;
      printf("selectivity was estimated to be %f\n", selectivity);

      if (selectivity <= 0.04) {
        // TODO: optimization for clustered index
        // vec_kp data;
        unsigned start_index;
        unsigned end_index;
        if (col->idx->form == BTREE) {
          // data = col->idx->form_data.btree->base_data;
          start_index = find(col->idx->form_data.btree, low);
          end_index = find(col->idx->form_data.btree, high);
        } else {
          // data = col->idx->form_data.sorted;
          start_index = find_sorted(col->idx->form_data.sorted, low);
          end_index = find_sorted(col->idx->form_data.sorted, high);
        }

        printf("start and end indices %du, %du\n", start_index, end_index);

        select_var->idx_select = malloc(sizeof(idx_select_variable));
        select_var->idx_select->index_low = start_index;
        select_var->idx_select->index_high = end_index;
        select_var->idx_select->col = col;

        if (col->idx->type == UNCLUSTERED) {
          materialize_index_selection(select_var);
        }
        return;
      }
    }

    select_range(col->data, select_var_index, dbo.filter.low, dbo.filter.high);
  }
}

OPERATOR_EXECUTE_DEFINITION(reselect, ReSelectOperator) {
  ENSURE_TYPED_VAR_EXISTS(dbo.select_var, _SELECT, first_selection, context);
  ENSURE_FETCH_VAR_OR_COL_EXISTS(dbo.fetch_var, fetch_data, context);

  if (handle[0]) {
    Variable *second_selection = create_variable(context, _SELECT, handle);
    if (second_selection->data.select.idx_select != NULL) {
      materialize_index_selection(&second_selection->data.select);
    }

    if (first_selection->data.select.inverse) {
      reselect_inverse(fetch_data, first_selection->data.select.selection,
                       size_fetch_data,
                       &second_selection->data.select.selection, dbo.filter.low,
                       dbo.filter.high);
    } else {
      reselect(fetch_data, first_selection->data.select.selection,
               &second_selection->data.select.selection, dbo.filter.low,
               dbo.filter.high);
    }
  }
}

OPERATOR_EXECUTE_DEFINITION(fetch, FetchOperator) {
  ENSURE_COL_EXISTS(dbo.col_id, col);
  ENSURE_TYPED_VAR_EXISTS(dbo.select_var, _SELECT, selection, context);

  if (handle[0]) {
    Variable *fetch_var = create_variable(context, _FETCH, handle);
    fetch_var->data.fetch.col = col;

    if (selection->data.select.idx_select != NULL) {
      // SelectVariable sel_var = selection->data.select;
      // Column *sel_col = sel_var.idx_select->col;
      //
      // assert(sel_col->idx != NULL);
      // assert(sel_col->idx->type == CLUSTERED);
      //
      // vec_int data = get_arr_col_copy(&sel_col->idx->cluster_data.copies,
      // col->col_id).data; fetch_index_range(data,
      // sel_var.idx_select->index_low,
      //                   sel_var.idx_select->index_high,
      //                   &fetch_var->data.fetch.fetch);

      // TODO: find alternative solution
      printf("materializing clustered index...\n");
      materialize_index_selection(&selection->data.select);
    }

    if (selection->data.select.idx_select != NULL) {
      printf("ERROR: this is not good, selection should be materialized!\n");
    }

    if (selection->data.select.inverse) {
      fetch_inverse(col->data, selection->data.select.selection,
                    &fetch_var->data.fetch.fetch);
    } else {
      fetch(col->data, selection->data.select.selection,
            &fetch_var->data.fetch.fetch);
    }
  }
}

OPERATOR_EXECUTE_DEFINITION(insert, InsertOperator) {
  (void)context;
  (void)handle;

  ENSURE_TBL_EXISTS(dbo.tbl_id, tbl);

  for (size_t i = 0; i < tbl->ordered_columns->size; ++i) {
    if (get_vec_col(tbl->ordered_columns, i)->idx != NULL) {
      ERROR("ERROR: inserting when index declared not supported yet!\n");
    }
  }

  if (dbo.values.size != tbl->columns->size) {
    ERROR("ERROR: too many values!\n");
  }

  for (size_t i = 0; i < dbo.values.size; ++i) {
    push_vec_int(&get_vec_col(tbl->ordered_columns, i)->data,
                 get_vec_int(&dbo.values, i));
  }
}

OPERATOR_EXECUTE_DEFINITION(aggregate, AggregateOperator) {
  // ENSURE_FETCH_VAR_OR_COL_EXISTS(dbo.var, data, context);

  vec_int data;
  Variable *var_data = get_variable_force_type(context, _FETCH, dbo.var);
  if (!var_data) {
    Column *col = lookup_col_id(dbo.var);
    if (!col) {
      ERROR("ERROR: no column or variable `%s` of required type!\n", dbo.var);
    }
    data = col->data;
  } else {
    data = to_vec_int(var_data->data.fetch.fetch);
  }

  if (handle[0]) {
    Variable *var = create_variable(context, _AGGREGATE, handle);
    switch (dbo.type) {
    case AVG:
      var->data.aggregate.value.type = FLOAT;
      var->data.aggregate.value.data._double = avg(data);
      break;
    case MIN:
      var->data.aggregate.value.type = INT;
      var->data.aggregate.value.data._int = _min(data);
      break;
    case MAX:
      var->data.aggregate.value.type = INT;
      var->data.aggregate.value.data._int = _max(data);
      break;
    case SUM:
      var->data.aggregate.value.type = LONG;
      var->data.aggregate.value.data._long = sum(data);
      break;
    }
  }
}

OPERATOR_EXECUTE_DEFINITION(join, JoinOperator) {
  if (!handle) {
    return;
  }

  vec_str *results = tokenize(handle, ",");
  if (results->size != 2) {
    ERROR("ERROR: invalid handle!\n");
  }

  Variable *first = create_variable(context, _SELECT, get_vec_str(results, 0));
  Variable *second = create_variable(context, _SELECT, get_vec_str(results, 1));
  delete_vec_str(results);

  ENSURE_TYPED_VAR_EXISTS(dbo.first_fetch, _FETCH, first_fetch, context);
  ENSURE_TYPED_VAR_EXISTS(dbo.second_fetch, _FETCH, second_fetch, context);
  ENSURE_TYPED_VAR_EXISTS(dbo.first_select, _SELECT, first_select, context);
  ENSURE_TYPED_VAR_EXISTS(dbo.second_select, _SELECT, second_select, context);

  // Make sure selections aren't weird
  assert(first_select->data.select.inverse == false);
  assert(second_select->data.select.inverse == false);
  assert(first_select->data.select.idx_select == NULL);
  assert(second_select->data.select.idx_select == NULL);

  JoinResult res;
  res.first = &first->data.select.selection;
  res.second = &second->data.select.selection;

#ifdef FAST_JOIN
  ensure_thread_pool(context);
#endif

  switch (dbo.type) {
  case HASH_JOIN:
#ifndef FAST_JOIN
    hash_join(first_fetch->data.fetch.fetch,
              first_select->data.select.selection,
              second_fetch->data.fetch.fetch,
              second_select->data.select.selection, &res);
#endif
#ifdef FAST_JOIN
    fast_hash_join(first_fetch->data.fetch.fetch,
                   first_select->data.select.selection,
                   second_fetch->data.fetch.fetch,
                   second_select->data.select.selection, &res, context->pool);
#endif
    break;
  case NESTED_LOOP_JOIN:
    nested_loop_join(first_fetch->data.fetch.fetch,
                     first_select->data.select.selection,
                     second_fetch->data.fetch.fetch,
                     second_select->data.select.selection, &res);
    break;
  case GRACE_HASH_JOIN:
    grace_hash_join(first_fetch->data.fetch.fetch,
                    first_select->data.select.selection,
                    second_fetch->data.fetch.fetch,
                    second_select->data.select.selection, &res);
    break;
  }
}

OPERATOR_EXECUTE_DEFINITION(combine, CombineOperator) {
  ENSURE_FETCH_VAR_OR_COL_EXISTS(dbo.first, first, context);
  ENSURE_FETCH_VAR_OR_COL_EXISTS(dbo.second, second, context);

  Variable *fetch_var = create_variable(context, _FETCH, handle);

  switch (dbo.type) {
  case ADD:
    add(first, second, &fetch_var->data.fetch.fetch);
    break;
  case SUB:
    sub(first, second, &fetch_var->data.fetch.fetch);
    break;
  }
}

void print_aggregate(AggregateVariable var, message *status) {
  switch (var.value.type) {
  case INT:
    message_log(status, "%d", var.value.data._int);
    break;
  case LONG:
    message_log(status, "%ld", var.value.data._long);
    break;
  case FLOAT:
    message_log(status, "%.2f", round(100.0 * var.value.data._double) / 100.0);
    break;
  }
}

void print_time(TimeVariable time, message *status) {
  double milliseconds = (double)(time.value.tv_usec) / 1000;
  int seconds = time.value.tv_sec;

  if (milliseconds < 0) {
    seconds--;
    milliseconds += 1000.0;
  }

  message_log(status, "%f", 1000.0 * seconds + milliseconds);
}

OPERATOR_EXECUTE_DEFINITION(print, PrintOperator) {
  (void)handle;
  (void)context;

  for (size_t i = 0; i < dbo.params.size; ++i) {
    char *name = get_vec_str(&dbo.params, i);
    ENSURE_VAR_EXISTS(name, var, context);

    switch (var->type) {
    case _SELECT:
      if (var->data.select.inverse) {
        message_log(status, "inverse %lu", var->data.select.selection.size);
      } else {
        message_log(status, "%lu", var->data.select.selection.size);
      }
      break;
    case _FETCH:
      for (size_t j = 0; j < var->data.fetch.fetch.size; ++j) {
        message_log(status, "%d\n", get_arr_int(&var->data.fetch.fetch, j));
      }
      break;
    case _AGGREGATE:
      print_aggregate(var->data.aggregate, status);
      break;
    case _TIME:
      print_time(var->data.time, status);
      break;
    }

    if (i != dbo.params.size - 1) {
      message_log(status, ",");
    }
  }
  message_log(status, "\n");
}

OPERATOR_EXECUTE_DEFINITION(batch_query, void *) {
  (void)dbo;
  (void)handle;

  // Start up thread pool if not already started
  ensure_thread_pool(context);

  if (active_batch(context)) {
    ERROR("ERROR: already an active operator batch!\n");
  }

  // Poke the batch to start queue
  push_batch(context, NULL);
}

OPERATOR_EXECUTE_DEFINITION(batch_execute, void *) {
  (void)dbo;
  (void)handle;

  if (!active_batch(context)) {
    ERROR("ERROR: must call `batch_query` before `batch_execute`!\n");
  }

  execute_batch(context);
  clear_batch(context);
}

int startup_db() {
  FILE *catalog_file = fopen(DB_CATALOG_FILE, "r");

  if (!catalog_file) {
    return 0;
  }

  // Read file
  size_t size;
  fseek(catalog_file, 0, SEEK_END);
  size = ftell(catalog_file);
  fseek(catalog_file, 0, SEEK_SET);

  char *catalog_content = malloc(sizeof(char) * size);
  assert(size == fread(catalog_content, sizeof(char), size, catalog_file));
  catalog_content[size - 1] = (char)0;

  vec_str *db_split = tokenize(catalog_content, "|");
  if (db_split->size >= 1) {
    current_db = new_db(get_vec_str(db_split, 0));
  }

  char file_name[1024];

  if (db_split->size == 2) {
    vec_str *tbls = tokenize(get_vec_str(db_split, 1), ",");
    delete_vec_str(db_split);

    for (size_t i = 0; i < tbls->size; ++i) {
      vec_str *tbl_split = tokenize(get_vec_str(tbls, i), "[");
      char *tbl_name = get_vec_str(tbl_split, 0);
      vec_str *columns = tokenize(get_vec_str(tbl_split, 1), ".");
      delete_vec_str(tbl_split);

      Table *tbl = new_table(tbl_name, columns->size);

      for (size_t j = 0; j < columns->size; ++j) {
        char *column_name = get_vec_str(columns, j);
        char *parse_col_name =
            (column_name[0] == '-') ? &column_name[3] : column_name;

        Column *col = new_column(parse_col_name);
        col->tbl = tbl;

        if (column_name[0] == '-') {
          index_type type = (column_name[1] == 'c') ? CLUSTERED : UNCLUSTERED;
          index_form form = (column_name[2] == 'b') ? BTREE : SORTED;
          col->idx = create_index(form, type, col);
        }

        sprintf(file_name, "cs165_data/%s.%s.%s", current_db->name, tbl->name,
                col->name);

        int fd = open(file_name, O_RDONLY, 0644);
        if (fd != -1) {
          struct stat stats;
          fstat(fd, &stats);
          size_t size = stats.st_size;

          int *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
          if (data != MAP_FAILED) {
            free(col->data.data);
            size_t vec_size = size / sizeof(int);
            size_t vec_capacity = vec_size + vec_size / 2;

            col->data.data = calloc(sizeof(int), vec_capacity);
            memcpy(col->data.data, data, size);
            col->data.size = vec_size;
            col->data.capacity = vec_capacity;
            col->tbl = tbl;

            munmap(data, size);
            close(fd);
          }
        }
        put_strh(tbl->columns, parse_col_name, col);
        col->col_id = tbl->ordered_columns->size;
        push_vec_col(tbl->ordered_columns, col);
      }

      // Populate indices
      for (size_t i = 0; i < tbl->columns->size; ++i) {
        Column *col = get_vec_col(tbl->ordered_columns, i);
        if (col->idx != NULL) {
          initialize_index(col->idx, col);
        }
      }

      delete_vec_str(columns);
      put_strh(current_db->tables, tbl_name, tbl);
    }

    delete_vec_str(tbls);
  }

  free(catalog_content);
  return 0;
}

OPERATOR_EXECUTE_DEFINITION(shutdown, void *) {
  (void)dbo;
  (void)status;
  (void)handle;
  (void)context;

  FILE *catalog_file = fopen(DB_CATALOG_FILE, "w+");
  mkdir("cs165_data", 0777);

  message catalog;
  alloc_message(&catalog);

  char file_name[1024];

  if (current_db) {
    message_log(&catalog, "%s|", current_db->name);
    FOR_STRH_START(current_db->tables, tbl, Table) {
      message_log(&catalog, "%s[", tbl->name);
      FOR_STRH_START(tbl->columns, col, Column) {
        if (col->idx != NULL) {
          message_log(&catalog, "-%c%c%s.",
                      col->idx->type == CLUSTERED ? 'c' : 'u',
                      col->idx->form == BTREE ? 'b' : 's', col->name);
        } else {
          message_log(&catalog, "%s.", col->name);
        }

        // MMAP column
        sprintf(file_name, "cs165_data/%s.%s.%s", current_db->name, tbl->name,
                col->name);

        size_t file_size = col->data.size * sizeof(int);
        remove(file_name);
        int fd = open(file_name, O_RDWR | O_CREAT, 0644);

        assert(fd != -1);
        lseek(fd, file_size - 1, SEEK_SET);
        assert(write(fd, "", 1) != -1);

        int *buffer =
            mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        memcpy(buffer, col->data.data, file_size);

        close(fd);
        munmap(buffer, file_size);

        if (col->idx != NULL) {
          sprintf("cs165_data/%s.%s.%s-%s.%s", current_db->name, tbl->name,
                  col->name,
                  col->idx->type == CLUSTERED ? "clustered" : "unclustered",
                  col->idx->form == BTREE ? "btree" : "shared");
        }
      }
      FOR_STRH_END
      message_log(&catalog, ",");
    }
    FOR_STRH_END
  }

  assert(catalog.payload != NULL);
  fputs(catalog.payload, catalog_file);
  free_message(&catalog);
  fclose(catalog_file);
}

// [CRITICAL] performance
void partial_load(csv_load_data *data, char *buffer) {
  if (data->cols == NULL) {
    size_t split_index = strcspn(buffer, "\n");

    // Remove trailing "\n"
    buffer[split_index] = 0;
    char *line = buffer;
    buffer = &buffer[split_index + 1];

    // Get vector of columns
    vec_str *col_ids = tokenize(line, ",");
    data->cols = new_vec_col(col_ids->size);
    for (size_t i = 0; i < col_ids->size; ++i) {
      char *col_id = get_vec_str(col_ids, i);
      Column *col = lookup_col_id(col_id);
      assert(col != NULL);

      // Clear column data
      free_vec_int(&col->data);
      alloc_vec_int(&col->data, INITIAL_COL_CAPACITY);

      push_vec_col(data->cols, col);
    }
    delete_vec_str(col_ids);

    data->col_index = 0;
    data->current = get_vec_col(data->cols, 0);
    data->result = 0;
    data->negative = 0;
  }

  for (size_t i = 0; buffer[i] != 0; ++i) {
    char c = buffer[i];

    if (c == '-') {
      data->negative = 1;
    } else if (c == ',') {
      if (data->negative) {
        data->result = -data->result;
      }

      fast_push_vec_int(&data->current->data, data->result, 2);
      data->current = get_vec_col(data->cols, ++data->col_index);
      data->result = 0;
      data->negative = 0;
    } else if (c == '\n') {
      if (data->negative) {
        data->result = -data->result;
      }

      fast_push_vec_int(&data->current->data, data->result, 2);
      data->col_index = 0;
      data->current = get_vec_col(data->cols, 0);
      data->result = 0;
      data->negative = 0;
    } else if (c != ' ') {
      data->result = 10 * data->result + (c - '0');
    }
  }
}

void initialize_indices(vec_col *cols) {
  // Populate indices
  for (size_t i = 0; i < cols->size; ++i) {
    Column *col = get_vec_col(cols, i);
    if (col->idx != NULL) {
      initialize_index(col->idx, col);
    }
  }
}

OPERATOR_EXECUTE_DEFINITION(load, LoadOperator) {
  (void)handle;
  (void)context;

  // Shouldn't run
  assert(false);

  // Check if file exists
  FILE *fd = fopen(dbo.file, "r");
  if (!fd) {
    ERROR("ERROR: file `%s` not found!\n", dbo.file);
  }

  // Read the first line
  char buffer[1024];
  if (!fgets(buffer, sizeof(buffer), fd)) {
    fclose(fd);
    ERROR("ERROR: file format error!\n");
  }

  // Remove trailing "\n"
  buffer[strcspn(buffer, "\n")] = 0;

  // Get vector of columns
  vec_str *col_ids = tokenize(buffer, ",");
  vec_ptr *cols = new_vec_ptr(col_ids->size);
  for (size_t i = 0; i < col_ids->size; ++i) {
    char *col_id = get_vec_str(col_ids, i);
    Column *col = lookup_col_id(col_id);

    if (!col) {
      delete_vec_str(col_ids);
      delete_vec_ptr(cols);
      fclose(fd);
      ERROR("ERROR: invalid col_id `%s`!", col_id);
    }

    // Clear column data
    free_vec_int(&col->data);
    alloc_vec_int(&col->data, INITIAL_COL_CAPACITY);

    push_vec_ptr(cols, col);
  }
  delete_vec_str(col_ids);

#ifndef FAST_LOAD
  vec_str *parsed = new_vec_str(cols->size);
  while (fgets(buffer, sizeof(buffer), fd)) {
    buffer[strcspn(buffer, "\n")] = 0;

    re_tokenize(parsed, buffer, ",");

    if (parsed->size != cols->size) {
      delete_vec_ptr(cols);
      delete_vec_str(parsed);
      ERROR("ERROR: file format error!\n");
    }

    for (size_t i = 0; i < cols->size; ++i) {
      Column *col = get_vec_ptr(cols, i);
      push_vec_int(&col->data, atoi(get_vec_str(parsed, i)));
    }
  }
  delete_vec_str(parsed);
#endif

  // WARNING! Assumes correct format, otherwise undefined behavior will occur
#ifdef FAST_LOAD
  while (fgets(buffer, sizeof(buffer), fd)) {
    size_t col_index = 0;
    Column *current = get_vec_ptr(cols, col_index);
    int result = 0;
    int negative = 0;

    for (size_t i = 0; i < sizeof(buffer); ++i) {
      char c = buffer[i];

      if (c == '-') {
        negative = 1;
      } else if (c == ',' || c == '\n') {
        if (negative) {
          result = -result;
        }

        push_vec_int(&current->data, result);
        current = get_vec_ptr(cols, ++col_index);
        result = 0;
        negative = 0;

        if (c == '\n') {
          break;
        }
      } else if (c != ' ') {
        result = 10 * result + (c - '0');
      }
    }
  }
#endif
  fclose(fd);

  // Populate indices
  for (size_t i = 0; i < cols->size; ++i) {
    Column *col = get_vec_ptr(cols, i);
    if (col->idx != NULL) {
      initialize_index(col->idx, col);
    }
  }

  delete_vec_ptr(cols);
}

OPERATOR_EXECUTE_DEFINITION(time, TimeOperator) {
  if (handle[0]) {
    struct timeval now;
    gettimeofday(&now, NULL);

    if (dbo.since_var[0]) {
      ENSURE_TYPED_VAR_EXISTS(dbo.since_var, _TIME, since, context);

      Variable *result = create_variable(context, _TIME, handle);
      struct timeval since_time = since->data.time.value;

      result->data.time.value.tv_sec = now.tv_sec - since_time.tv_sec;
      result->data.time.value.tv_usec = now.tv_usec - since_time.tv_usec;
    } else {
      Variable *result = create_variable(context, _TIME, handle);
      result->data.time.value = now;
    }
  }
}

OPERATOR_EXECUTE_DEFINITION(info, void *) {
  (void)dbo;
  (void)handle;
  (void)context;

  if (current_db) {
    message_log(status, "database server info:\n");
    message_log(status, "db: `%s`\n", current_db->name);

    FOR_STRH_START(current_db->tables, tbl, Table) {
      message_log(status, "  tbl: `%s`\n", tbl->name);

      FOR_STRH_START(tbl->columns, col, Column) {
        message_log(status, "    col: `%s`, size %lu\n", col->name,
                    col->data.size);
        if (col->idx != NULL) {
          message_log(status, "       has a %s %s index\n",
                      col->idx->type == CLUSTERED ? "clustered" : "unclustered",
                      col->idx->form == BTREE ? "btree" : "sorted");
        }
      }
      FOR_STRH_END
    }
    FOR_STRH_END
  }
}

void execute_operator(Operator *dbo, ClientContext *context, message *status) {
  assert(dbo);

  if (active_batch(context) && dbo->type != BATCH_EXECUTE) {
    if (dbo->type != SELECT && dbo->type != RESELECT &&
        dbo->type != AGGREGATE && dbo->type != COMBINE && dbo->type != FETCH) {
      ERROR("ERROR: cannot have this type of operator in a batch!\n");
    }

    push_batch(context, dbo);
    return;
  }

  switch (dbo->type) {
    OPERATOR_EXECUTE(CREATE, create);
    OPERATOR_EXECUTE(LOAD, load);
    OPERATOR_EXECUTE(FETCH, fetch);
    OPERATOR_EXECUTE(SELECT, select);
    OPERATOR_EXECUTE(RESELECT, reselect);
    OPERATOR_EXECUTE(INSERT, insert);
    OPERATOR_EXECUTE(PRINT, print);
    OPERATOR_EXECUTE(AGGREGATE, aggregate);
    OPERATOR_EXECUTE(COMBINE, combine);
    OPERATOR_EXECUTE(SHUTDOWN, shutdown);
    OPERATOR_EXECUTE(INFO, info);
    OPERATOR_EXECUTE(BATCH_QUERY, batch_query);
    OPERATOR_EXECUTE(BATCH_EXECUTE, batch_execute);
    OPERATOR_EXECUTE(TIME, time);
    OPERATOR_EXECUTE(JOIN, join);
  }

  free_operator(dbo);
}

void free_all_db() { delete_db(current_db); }
