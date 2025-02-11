#pragma once

#include "batch.h"

#define DB_CATALOG_FILE "cs165.catalog"

#define SERVER_MIRROR_CLIENT

// Fast parse (without checking for validity)
#define FAST_LOAD

// Override for number of threads in the thread pool (default number of processors)
// #define DEFAULT_THREAD_COUNT            16

#define ENABLE_MULTITHREADING

// Execution strategy for batches
#define DEFAULT_BATCH_HEURISTIC         BREADTH_SELECT_HEURISTIC 

// Max number of slices to partition a column into 
#define MAX_COLUMN_PARTITION_SIZE       256

// Minimum size of a partition slice in bytes
#define MIN_COLUMN_PARTITION_CHUNK_SIZE (128 * 4096)

#define USE_SELECTIVITY_ESTIMATOR
#define SELECTIVITY_ESTIMATOR_PAGE_SIZE 4096
#define MAX_SELECTIVITY_ESTIMATOR_SAMPLES 16

// Growth rate for vectors in a multiselect
#define SELECT_GROWTH_RATE 10

// Starting size for selection variables
#define INITIAL_SELECTION_CAPACITY      1024

// Starting size for fetch variables
#define INITIAL_FETCH_CAPACITY          1024

// Join
#define NESTED_LOOP_BLOCK_SIZE          (8 * 4096)
// #define FAST_JOIN
