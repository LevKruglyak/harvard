#include "pool_alloc.h"

#include <stdio.h>
#include <stdlib.h>

pool_alloc *new_pool(int chunk_size, int block_size, int initial_size) {
  pool_alloc *alloc = malloc(sizeof(pool_alloc));
  if (alloc == NULL) {
    return NULL;
  }

  block *_block = new_block(chunk_size, initial_size);
  if (_block == NULL) {
    free(alloc);
    return NULL;
  }

  alloc->chunk_size = chunk_size;
  alloc->block_size = block_size;
  alloc->alloc_ptr = (chunk *)_block->start;
  alloc->blocks = _block;

  return alloc;
}

void delete_pool(pool_alloc *alloc) {
  // Free blocks
  block *current = alloc->blocks;
  while (current != NULL) {
    block *next = current->next;
    free(current->start);
    free(current);
    current = next;
  }

  free(alloc);
}

block *new_block(int chunk_size, int block_size) {
  block *_block = malloc(sizeof(block));
  if (_block == NULL) {
    return NULL;
  }

  void *chunks = malloc(block_size * (sizeof(chunk) + chunk_size));
  if (chunks == NULL) {
    free(_block);
    return NULL;
  }

  chunk *start = (chunk *)chunks;

  // Set up linked list structure
  chunk *current = start;
  for (int index = 0; index < block_size - 1; ++index) {
    current->next = (chunk *)((void *)current + (sizeof(chunk) + chunk_size));
    current = current->next;
  }
  current->next = NULL;

  _block->start = chunks;
  _block->next = NULL;

  return _block;
}

void *malloc_chunk(pool_alloc *alloc) {
  if (alloc->alloc_ptr == NULL) {
    // Allocate more space
    block *_block = new_block(alloc->chunk_size, alloc->block_size);
    if (_block == NULL) {
      return NULL;
    }

    // Append block to list
    _block->next = alloc->blocks;
    alloc->blocks = _block;

    alloc->alloc_ptr = (chunk *)_block->start;
  }

  void *payload = (void *)alloc->alloc_ptr + sizeof(chunk);
  alloc->alloc_ptr = alloc->alloc_ptr->next;

  return payload;
}

void free_chunk(pool_alloc *alloc, void *payload) {
  chunk *payload_chunk = (chunk *)(payload - sizeof(chunk));
  payload_chunk->next = alloc->alloc_ptr;
  alloc->alloc_ptr = payload_chunk;
}
