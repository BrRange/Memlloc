#include "arena.h"
#include "pool.h"
#include "slide.h"

Arena arena_new(u32 bytes){
  bytes = (bytes + 7) & ~7;
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
  chkSize = (chkSize + 7) & ~7;
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

Slide slide_new(u32 bytes){
  bytes = (bytes + 7) & ~7;
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