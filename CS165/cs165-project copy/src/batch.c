#include "include/batch.h"
#include "include/client_context.h"
#include "include/db_internals.h"
#include "include/dep_graph.h"
#include "include/multiselect.h"
#include "include/operators.h"
#include "include/thread_pool.h"
#include "include/vector.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VECTOR_IMPL(gen_var *, gen_var)
VECTOR_IMPL(dep_link, dep_link)

dep_link *new_dep_link() {
  dep_link *dl = malloc(sizeof(dep_link));
  return dl;
}

void delete_dep_link(dep_link *dl) { free(dl); }

gen_var *new_gen_var(gen_var_type type, dep_link *parent) {
  gen_var *gv = malloc(sizeof(gen_var));
  gv->parent = parent;
  gv->type = type;
  gv->depth = -1;
  return gv;
}

void delete_gen_var(gen_var *gv) {
  delete_dep_link(gv->parent);
  free(gv);
}

void calculate_depth(gen_var *gv) {
  // If depth has not already been calculated
  if (gv->depth != -1) {
    return;
  }

  gv->depth = 0;
  gen_var *parents[2];
  size_t parents_size = 0;
  switch (gv->parent->type) {
  case SELECT_LINK:
    parents[0] = gv->parent->data.select.col;
    parents_size = 1;
    break;
  case RESELECT_LINK:
    parents[0] = gv->parent->data.reselect.col;
    parents[1] = gv->parent->data.reselect.sel;
    parents_size = 2;
    break;
  case FETCH_LINK:
    parents[0] = gv->parent->data.fetch.col;
    parents[1] = gv->parent->data.fetch.sel;
    parents_size = 2;
    break;
  case AGGREGATE_LINK:
    parents[0] = gv->parent->data.aggregate.col;
    parents_size = 1;
    break;
  case COMBINE_LINK:
    parents[0] = gv->parent->data.combine.first;
    parents[1] = gv->parent->data.combine.second;
    parents_size = 2;
    break;
  }

  for (size_t i = 0; i < parents_size; ++i) {
    calculate_depth(parents[i]);
    int compare_depth = 1 + parents[i]->depth;

    if (compare_depth > gv->depth) {
      gv->depth = compare_depth;
    }
  }
}

gen_var *add_new_var_to_dp(dep_graph *dp, ClientContext *context,
                           VariableType type, char *handle, dep_link *parent) {
  Variable *var = create_variable(context, type, handle);
  assert(var);
  gen_var *gv = new_gen_var(GV_VARIABLE, parent);
  gv->data.var = var;
  strcpy(gv->_name, handle);
  calculate_depth(gv);
  push_vec_gen_var(dp->variables, gv);
  return gv;
}

gen_var *add_col_to_dp(dep_graph *dp, char *handle) {
  Column *col = lookup_col_id(handle);
  assert(col);
  for (size_t i = 0; i < dp->variables->size; ++i) {
    gen_var *gv_compare = get_vec_gen_var(dp->variables, i);
    if (gv_compare->type == GV_COLUMN && gv_compare->data.col == col) {
      return gv_compare;
    }
  }

  gen_var *gv = new_gen_var(GV_COLUMN, NULL);
  gv->data.col = col;
  gv->depth = 0;
  strcpy(gv->_name, handle);
  push_vec_gen_var(dp->variables, gv);
  return gv;
}

gen_var *add_var_to_dp(dep_graph *dp, ClientContext *context, char *handle) {
  Variable *var = get_variable(context, handle);
  if (var == NULL) {
    return add_col_to_dp(dp, handle);
  }

  for (size_t i = 0; i < dp->variables->size; ++i) {
    gen_var *gv_compare = get_vec_gen_var(dp->variables, i);
    if (gv_compare->type == GV_VARIABLE && gv_compare->data.var == var) {
      return gv_compare;
    }
  }

  gen_var *gv = new_gen_var(GV_VARIABLE, NULL);
  gv->data.var = var;
  gv->depth = 0;
  strcpy(gv->_name, handle);
  push_vec_gen_var(dp->variables, gv);
  return gv;
}

void add_operator_to_dp(dep_graph *dp, ClientContext *context, Operator *dbo) {
  if (!dbo->handle[0]) {
    return;
  }

  dep_link *dl = new_dep_link();
  switch (dbo->type) {
  case SELECT:
    dl->type = SELECT_LINK;
    dl->data.select.filter = dbo->fields.select.filter;
    dl->data.select.col = add_var_to_dp(dp, context, dbo->fields.select.col_id);
    dl->out = add_new_var_to_dp(dp, context, _SELECT, dbo->handle, dl);
    break;
  case RESELECT:
    dl->type = RESELECT_LINK;
    dl->data.reselect.filter = dbo->fields.reselect.filter;
    dl->data.reselect.col =
        add_var_to_dp(dp, context, dbo->fields.reselect.fetch_var);
    dl->data.reselect.sel =
        add_var_to_dp(dp, context, dbo->fields.reselect.select_var);
    dl->out = add_new_var_to_dp(dp, context, _SELECT, dbo->handle, dl);
    break;
  case FETCH:
    dl->type = FETCH_LINK;
    dl->data.fetch.col = add_var_to_dp(dp, context, dbo->fields.fetch.col_id);
    dl->data.fetch.sel =
        add_var_to_dp(dp, context, dbo->fields.fetch.select_var);
    dl->out = add_new_var_to_dp(dp, context, _FETCH, dbo->handle, dl);
    break;
  case AGGREGATE:
    dl->type = AGGREGATE_LINK;
    dl->data.aggregate.type = dbo->fields.aggregate.type;
    dl->data.aggregate.col =
        add_var_to_dp(dp, context, dbo->fields.aggregate.var);
    dl->out = add_new_var_to_dp(dp, context, _AGGREGATE, dbo->handle, dl);
    break;
  case COMBINE:
    dl->type = COMBINE_LINK;
    dl->data.combine.type = dbo->fields.combine.type;
    dl->data.combine.first =
        add_var_to_dp(dp, context, dbo->fields.combine.first);
    dl->data.combine.second =
        add_var_to_dp(dp, context, dbo->fields.combine.second);
    dl->out = add_new_var_to_dp(dp, context, _FETCH, dbo->handle, dl);
    break;
  default:
    // Should never reach here
    assert(0);
    break;
  }

  free_operator(dbo);
}

dep_graph *create_dep_graph() {
  dep_graph *dp = malloc(sizeof(dep_graph));
  dp->variables = new_vec_gen_var(10);
  return dp;
}

static inline vec_int *get_col_data(gen_var *var) {
  assert(var);
  assert(var->type == GV_COLUMN);
  return &var->data.col->data;
}

static inline vec_index *get_select_data(gen_var *var) {
  assert(var);
  assert(var->type == GV_VARIABLE);
  assert(var->data.var->type == _SELECT);
  return &var->data.var->data.select.selection;
}

static inline arr_int *get_fetch_data(gen_var *var) {
  assert(var);
  assert(var->type == GV_VARIABLE);
  assert(var->data.var->type == _FETCH);
  return &var->data.var->data.fetch.fetch;
}

static inline vec_int get_col_or_fetch_data(gen_var *var) {
  assert(var);
  if (var->type == GV_COLUMN) {
    return *get_col_data(var);
  } else {
    return to_vec_int(*get_fetch_data(var));
  }
}

void execute_full_link(dep_link *link) {
  switch (link->type) {
  case SELECT_LINK:
    select_range(get_col_or_fetch_data(link->data.select.col),
                 get_select_data(link->out), link->data.select.filter.low,
                 link->data.select.filter.high);
    break;
  case RESELECT_LINK:
    reselect(get_col_or_fetch_data(link->data.reselect.col),
             *get_select_data(link->data.reselect.sel),
             get_select_data(link->out), link->data.reselect.filter.low,
             link->data.reselect.filter.high);
    break;
  case FETCH_LINK:
    fetch(get_col_or_fetch_data(link->data.fetch.col),
          *get_select_data(link->data.fetch.sel), get_fetch_data(link->out));
    break;
  case AGGREGATE_LINK: {
    Variable *var = link->out->data.var;
    vec_int col = get_col_or_fetch_data(link->data.aggregate.col);
    switch (link->data.aggregate.type) {
    case AVG:
      var->data.aggregate.value.type = FLOAT;
      var->data.aggregate.value.data._double = avg(col);
      break;
    case MIN:
      var->data.aggregate.value.type = INT;
      var->data.aggregate.value.data._int = _min(col);
      break;
    case MAX:
      var->data.aggregate.value.type = INT;
      var->data.aggregate.value.data._int = _max(col);
      break;
    case SUM:
      var->data.aggregate.value.type = INT;
      var->data.aggregate.value.data._int = sum(col);
      break;
    }
  } break;
  case COMBINE_LINK: {
    vec_int first = get_col_or_fetch_data(link->data.combine.first);
    vec_int second = get_col_or_fetch_data(link->data.combine.second);
    Variable *out = link->out->data.var;
    switch (link->data.combine.type) {
    case ADD:
      add(first, second, &out->data.fetch.fetch);
      break;
    case SUB:
      sub(first, second, &out->data.fetch.fetch);
      break;
    }
  } break;
  }
}

void thread_execute_fill_link(void *args) { execute_full_link(args); }

typedef int (*comparison_func)(gen_var *, gen_var *);
void sort_dep_graph(dep_graph *dp, comparison_func func) {
  // Sort variable list by depth (insertion sort)
  gen_var **vars = dp->variables->data;
  for (int i = 1; i < (int)dp->variables->size; ++i) {
    gen_var *gv = vars[i];
    int j = i - 1;

    while (j >= 0 && func(vars[j], gv)) {
      vars[j + 1] = vars[j];
      j--;
    }

    vars[j + 1] = gv;
  }
}

int depth_comparison(gen_var *gv_1, gen_var *gv_2) {
  return gv_1->depth > gv_2->depth;
}

void execute_breadth_heuristic(ClientContext *context, dep_graph *dp) {
  // Step 1: Sort by depth
  sort_dep_graph(dp, depth_comparison);

  // Step 2: Execute each depth level in parallel
  int stage = 0;
  for (size_t i = 0; i < dp->variables->size; ++i) {
    gen_var *gv = get_vec_gen_var(dp->variables, i);
    if (stage != gv->depth) {
#ifdef ENABLE_MULTITHREADING
      // Move onto next stage
      wait_thread_pool(context->pool);
#endif
    }

    stage = gv->depth;
    if (stage != 0) {
      assert(gv->parent != NULL);
#ifdef ENABLE_MULTITHREADING
      add_task_thread_pool(context->pool, thread_execute_fill_link, gv->parent);
#endif
#ifndef ENABLE_MULTITHREADING
      thread_execute_fill_link(gv->parent);
#endif
    }
  }

#ifdef ENABLE_MULTITHREADING
  wait_thread_pool(context->pool);
#endif
}

int depth_select_comparison(gen_var *gv_1, gen_var *gv_2) {
  if (gv_1->depth > gv_2->depth) {
    return 1;
  }

  // Within a depth level, select links should go first
  if (gv_1->depth == gv_2->depth) {
    if (gv_1->depth == 0) {
      return 0;
    }

    if ((gv_1->parent->type == SELECT_LINK) &
        (gv_2->parent->type != SELECT_LINK))
      return 0;

    if ((gv_1->parent->type != SELECT_LINK) &
        (gv_2->parent->type == SELECT_LINK))
      return 1;

    if ((gv_1->parent->type == SELECT_LINK) &
        (gv_2->parent->type == SELECT_LINK))
      return gv_1->parent->data.select.col < gv_2->parent->data.select.col;
  }

  return 0;
}

VECTOR_IMPL(SelectVariable *, sel_var)

void execute_multiselect(ClientContext *context, dep_graph *dp, size_t start,
                         size_t end) {
  arr_sel_var selections;
  alloc_arr_sel_var(&selections, end - start);
  arr_filter filters;
  alloc_arr_filter(&filters, end - start);

  vec_int *column = NULL;
  for (size_t i = 0; i < end - start; ++i) {
    gen_var *gv = get_vec_gen_var(dp->variables, i + start);
    assert(gv->parent->type == SELECT_LINK);

    if (i != 0) {
      assert(get_col_data(gv->parent->data.select.col) == column);
    }
    column = get_col_data(gv->parent->data.select.col);

    set_arr_sel_var(&selections, &gv->data.var->data.select, i);
    set_arr_filter(&filters, convert_filter(gv->parent->data.select.filter), i);
  }

  assert(column != NULL);
  multiselect(context->pool, *column, filters, selections);

  free_arr_sel_var(&selections);
  free_arr_filter(&filters);
}

void execute_breadth_select_heuristic(ClientContext *context, dep_graph *dp) {
  // Step 1: Sort by depth
  sort_dep_graph(dp, depth_select_comparison);

  // Step 2: Sort within each depth level
  int stage = 0;
  gen_var *select_par = NULL;
  size_t start = 0;
  for (size_t i = 0; i < dp->variables->size; ++i) {
    gen_var *gv = get_vec_gen_var(dp->variables, i);
    if (stage != gv->depth) {
      if (select_par != NULL) {
        execute_multiselect(context, dp, start, i);
        select_par = NULL;
      }
      start = 0;

#ifdef ENABLE_MULTITHREADING
      // Move onto next stage
      wait_thread_pool(context->pool);
#endif
    }

    stage = gv->depth;
    if (stage != 0) {
      assert(gv->parent != NULL);

      if (gv->parent->type == SELECT_LINK) {
        if (select_par == NULL) {
          select_par = gv->parent->data.select.col;
          start = i;
          if (i == dp->variables->size - 1) {
            execute_multiselect(context, dp, start, start + 1);
            select_par = NULL;
          }
        } else if (select_par != gv->parent->data.select.col ||
                   i == dp->variables->size - 1) {
          execute_multiselect(context, dp, start,
                              i + (i == dp->variables->size - 1));
          select_par = gv->parent->data.select.col;
          start = i;
        }
      } else {
        if (select_par != NULL) {
          execute_multiselect(context, dp, start, i);
          select_par = NULL;
        }
        start = 0;

#ifndef ENABLE_MULTITHREADING
        thread_execute_fill_link(gv->parent);
#endif
#ifdef ENABLE_MULTITHREADING
        add_task_thread_pool(context->pool, thread_execute_fill_link,
                             gv->parent);
#endif
      }
    }
  }

#ifdef ENABLE_MULTITHREADING
  wait_thread_pool(context->pool);
#endif
}

void execute_no_heuristic(dep_graph *dp) {
  for (size_t i = 0; i < dp->variables->size; ++i) {
    gen_var *gv = get_vec_gen_var(dp->variables, i);
    if (gv->parent != NULL) {
      execute_full_link(gv->parent);
    }
  }
}

void execute_dep_graph(ClientContext *context, dep_graph *dp,
                       batch_heuristic heuristic) {
  switch (heuristic) {
  case NO_HEURISTIC:
    execute_no_heuristic(dp);
    break;
  case BREADTH_HEURISTIC:
    execute_breadth_heuristic(context, dp);
    break;
  case BREADTH_SELECT_HEURISTIC:
    execute_breadth_select_heuristic(context, dp);
    break;
  }
}

void destroy_dep_graph(dep_graph *dp) {
  for (size_t i = 0; i < dp->variables->size; ++i) {
    delete_gen_var(get_vec_gen_var(dp->variables, i));
  }
  delete_vec_gen_var(dp->variables);
  free(dp);
}
