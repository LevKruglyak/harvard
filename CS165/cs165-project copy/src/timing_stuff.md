# Things to measure
Milestone 1

1. Optimizing load
2. Bitvector vs position list

Milestone 2

1. number of threads in thread_pool (as a factor of number of processors)
2. parallel data vs parallel threads
3. various heuristics for tree execution

BREADTH_SELECT_HEURISTIC performs better than BREADTH_HEURISTIC for most cases
parallelism yields diminishing returns after about (proc_count) / 2
unwrapped loops with multiple selects don't give much benefit (L2 vs L3) optimization
for highly selective queries, larger blocks seem to work better

proposed optimization: estimate selectivity, if >0.5 do inverse selection

Milestone 3
