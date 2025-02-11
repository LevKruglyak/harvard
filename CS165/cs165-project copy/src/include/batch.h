#pragma once

#include "client_context.h"
#include "dep_graph.h"

VECTOR_HEAD(dep_link, dep_link)
VECTOR_HEAD(SelectVariable *, sel_var)
ARRAY_HEAD(SelectVariable *, sel_var)

dep_graph *create_dep_graph();
void add_operator_to_dp(dep_graph *dp, ClientContext *context, Operator *dbo);
void destroy_dep_graph(dep_graph *dep_graph);

typedef enum {
  // Execute operators in the order in which they were given (single threaded)
  NO_HEURISTIC,
  // Sort operators by dependency depth, execute each depth level in parallel using a thread pool
  BREADTH_HEURISTIC,
  // Same as regular breadth heuristic, but now we optimally execute the pattern: (one column, multiple selects)
  BREADTH_SELECT_HEURISTIC,
} batch_heuristic;

void execute_dep_graph(ClientContext *context, dep_graph *dep_graph, batch_heuristic heuristic);
