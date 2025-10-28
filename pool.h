#include <stdlib.h>
#include <stdint.h>

#ifndef MEMLLOC_BLOCK_
#define MEMLLOC_BLOCK_

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

#ifdef IMPLEMENT_

Pool pool_new(uint32_t chkSize, uint32_t chkCount){
  chkSize = chkSize % 8 ? 8ull - (chkSize % 8ull) + chkSize : chkSize;
  Pool pool = {
    .root = malloc(1ull * chkCount * chkSize),
    .ready = pool.root,
    .chkSize = chkSize,
    .chkCount = chkCount,
  };
  uint64_t chkOffset = chkSize / 8ull;
  Chunk *init = pool.root;
  for(uint32_t i = 1; i < chkCount; ++i){
    init->next = init + chkOffset;
    init = init->next;
  }
  init->next = NULL;
  return pool;
}

void *pool_alloc(Pool *pool){
  if(!pool->ready) return NULL;
  void *ptr = pool->ready;
  pool->ready = pool->ready->next;
  return ptr;
}

void pool_pop(Pool *pool, void *addr){
  Chunk *chk = addr;
  chk->next = pool->ready;
  pool->ready = chk;
}

void pool_free(Pool *pool){
  free(pool->root);
}

void pool_destroy(Pool *pool){
  free(pool->root);
  *pool = (Pool){0};
}

#endif
