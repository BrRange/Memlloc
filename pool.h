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
  u32 chkSize, chkCount;
} Pool;

Pool pool_new(u32 chkSize, u32 chkCount);

void *pool_alloc(Pool *pool);

void pool_pop(Pool *pool, void *addr);

void pool_free(Pool *pool);

void pool_destroy(Pool *pool);

typedef struct PoolLink{
  Pool pool;
  struct PoolLink *link;
} PoolLink;

PoolLink *poolLink_new(u32 chkSize, u32 chkCount);

void *poolLink_alloc(PoolLink *poolLink);

void poolLink_pop(PoolLink *poolLink, void *addr);

void poolLink_free(PoolLink *poolLink);

void poolLink_destroy(PoolLink *poolLink);

#endif