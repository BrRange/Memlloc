#include "arena.h"
#include "pool.h"
#include "slide.h"

#ifndef MEMLLOC_ALIGN
#define MEMLLOC_ALIGN(x) ((x + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1))
#endif

Arena arena_new(u32 bytes){
  bytes = MEMLLOC_ALIGN(bytes);
  Arena arn = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return arn;
}

void *arena_alloc(Arena *arn, u32 bytes){
  if(arn->cursor + bytes > arn->cap) return NULL;
  void *mem = arn->mem + arn->cursor;
  arn->cursor += bytes;
  return mem;
}

void arena_pop(Arena *arn, u32 bytes){
  if(bytes >= arn->cursor)
    arn->cursor = 0;
  else
    arn->cursor -= bytes;
}

void arena_free(Arena *arn){
  free(arn->mem);
}

void arena_destroy(Arena *arn){
  free(arn->mem);
  *arn = (Arena){0};
}

Pool pool_new(u32 chkSize, u32 chkCount){
  chkSize = MEMLLOC_ALIGN(chkSize);
  Pool pool = {
    .root = malloc(1ull * chkCount * chkSize),
    .ready = pool.root,
    .chkSize = chkSize,
    .chkCount = chkCount,
  };
  Chunk *init = pool.root;
  for(u32 i = 1; i < chkCount; ++i){
    init->next = (Chunk*)((u8*)init + pool.chkSize);
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

PoolLink *poolLink_new(u32 chkSize, u32 chkCount){
  PoolLink *poolLink = malloc(sizeof *poolLink);
  poolLink->pool = pool_new(chkSize, chkCount);
  poolLink->link = NULL;
  return poolLink;
}

void *poolLink_alloc(PoolLink *poolLink){
  void *mem = pool_alloc(&poolLink->pool);
  if(!mem){
    if(!poolLink->link)
      poolLink->link = poolLink_new(poolLink->pool.chkSize, poolLink->pool.chkCount * 2);
    return poolLink_alloc(poolLink->link);
  }
  return mem;
}

void poolLink_pop(PoolLink *poolLink, void *addr){
  isz offset = (Chunk*)addr - poolLink->pool.root;
  if(offset < 0 || offset >= poolLink->pool.chkCount){
    if(poolLink->link) poolLink_pop(poolLink->link, addr);
  }
  else pool_pop(&poolLink->pool, addr);
}

void poolLink_free(PoolLink *poolLink){
  PoolLink *next = poolLink->link;
  pool_free(&poolLink->pool);
  free(poolLink);
  if(next) poolLink_free(next);
}

void poolLink_destroy(PoolLink *poolLink){
  PoolLink *next = poolLink->link;
  pool_destroy(&poolLink->pool);
  poolLink->link = NULL;
  free(poolLink);
  if(next) poolLink_destroy(next);
}

Slide slide_new(u32 bytes){
  bytes = MEMLLOC_ALIGN(bytes);
  Slide sld = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return sld;
}

void *slide_alloc(Slide *sld, u32 bytes){
  if(sld->left >= bytes){
    sld->left -= bytes;
    return sld->mem + sld->left;
  }
  if(sld->right <= sld->cap - bytes){
    void *alloc = sld->mem + sld->right;
    sld->right += bytes;
    return alloc;
  }
  return NULL;
}

int slide_pop(Slide *sld, u32 index, u32 bytes){
  if(index == sld->left){
    sld->left += bytes;
    return 1;
  }
  if(index == sld->right - bytes){
    sld->right -= bytes;
    return 1;
  }
  return 0;
}

u32 slide_getIndex(Slide *sld, void *mem){
  return mem - sld->mem;
}

void *slide_getMem(Slide *sld, u32 index){
  return sld->mem + index;
}

void slide_free(Slide *sld){
  free(sld->mem);
}

void slide_destroy(Slide *sld){
  free(sld->mem);
  *sld = (Slide){0};
}