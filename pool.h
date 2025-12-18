#ifndef MEMLLOC_BLOCK_
#define MEMLLOC_BLOCK_

#include <stdlib.h>
#include <stdint.h>
#include "rustydef.h"

typedef struct Chunk{
  struct Chunk *next;
} Chunk;

typedef struct Pool{
  Chunk *root, *ready;
  uint32_t chkSize, chkCount;
} Pool;

Pool pool_new(uint32_t chkSize, uint32_t chkCount);

void *pool_alloc(Pool *pool);

void pool_pop(Pool *pool, void *addr);

void pool_free(Pool *pool);

void pool_destroy(Pool *pool);

#endif