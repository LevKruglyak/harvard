#include "include/variable.h"
#include "include/globals.h"
#include <stdio.h>
#include "include/qsort.h"

Variable *new_var(VariableType type) {
  Variable *var = calloc(sizeof(Variable), 1);
  var->type = type;
  switch (type) {
  case _SELECT:
    alloc_vec_index(&var->data.select.selection, INITIAL_SELECTION_CAPACITY);
    var->data.select.inverse = false;
    var->data.select.no_block = false;
    var->data.select.idx_select = NULL;
    break;
  case _FETCH:
    break;
  case _AGGREGATE:
    break;
  case _TIME:
    break;
  }
  return var;
}

void delete_var(Variable *var) {
  switch (var->type) {
  case _SELECT:
    free_vec_index(&var->data.select.selection);
    free(var->data.select.idx_select);
    break;
  case _FETCH:
    free_arr_int(&var->data.fetch.fetch);
    break;
  case _AGGREGATE:
    break;
  case _TIME:
    break;
  }
  free(var);
}

static inline int cmp_u(const void * a, const void * b)
{
  return ((*(unsigned *)a) - (*(unsigned *)b));
}

void materialize_index_selection(SelectVariable *var) {
  if (var->idx_select != NULL) {
    Column *col = var->idx_select->col;
    assert(col->idx != NULL);
    vec_kp base = col->idx->form == BTREE ? col->idx->form_data.btree->base_data
                                          : col->idx->form_data.sorted;

    for (size_t i = var->idx_select->index_low; i < var->idx_select->index_high;
         ++i) {
      fast_push_vec_index(&var->selection, get_vec_kp(&base, i).pos,
                          SELECT_GROWTH_RATE);
    }

    // Sort the selection
    QSORT(unsigned, var->selection.data, var->selection.size, cmp_u);
    free(var->idx_select);
    var->idx_select = NULL;
  } else {
    printf("this shouldn't happen...\n");
  }
}

VECTOR_IMPL(Variable *, var)
