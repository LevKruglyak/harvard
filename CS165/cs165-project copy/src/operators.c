#include "include/operators.h"
#include "include/vector.h"

void free_operator(Operator *dbo) {
  switch (dbo->type) {
    case INSERT:
      free_vec_int(&dbo->fields.insert.values);
      break;
    case PRINT:
      free_vec_str(&dbo->fields.print.params);
      break;
    default:
      break;
  }

  free(dbo);
}

VECTOR_IMPL(Operator *, oper)
