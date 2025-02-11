#ifndef POOL_ALLOC
#define POOL_ALLOC

// Single chunk of allocated data
typedef struct chunk {
  struct chunk *next;
} chunk;

// Used for garbage collection
typedef struct block {
  void *start;
  struct block *next;
} block;

typedef struct pool_alloc {
  block *blocks;
  int block_size;
  int chunk_size;
  chunk *alloc_ptr;
} pool_alloc;

pool_alloc *new_pool(int chunk_size, int block_size, int initial_size);
void delete_pool(pool_alloc *alloc);

block *new_block(int chunk_size, int block_size);

void *malloc_chunk(pool_alloc *alloc);
void free_chunk(pool_alloc *alloc, void *payload);

#endif
