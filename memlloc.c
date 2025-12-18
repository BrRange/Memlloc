#include "arena.h"
#include "pool.h"
#include "slide.h"

Arena arena_new(uint32_t bytes){
  bytes = bytes % 8 ? 8ull - (bytes % 8ull) + bytes : bytes;
  Arena arn = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return arn;
}

void *arena_alloc(Arena *arn, uint32_t bytes){
  if(arn->cursor + bytes > arn->cap) return NULL;
  void *mem = arn->mem + arn->cursor;
  arn->cursor += bytes;
  return mem;
}

void arena_pop(Arena *arn, uint32_t bytes){
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

Slide slide_new(uint32_t bytes){
  bytes = bytes % 8 ? 8ull - (bytes % 8ull) + bytes : bytes;
  Slide sld = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return sld;
}

void *slide_alloc(Slide *sld, uint32_t bytes){
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

int slide_pop(Slide *sld, uint32_t index, uint32_t bytes){
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

uint32_t slide_getIndex(Slide *sld, void *mem){
  uint8_t *byteMem = mem;
  return byteMem - sld->mem;
}

void *slide_getMem(Slide *sld, uint32_t index){
  return sld->mem + index;
}

void slide_free(Slide *sld){
  free(sld->mem);
}

void slide_destroy(Slide *sld){
  free(sld->mem);
  *sld = (Slide){0};
}