#include "arena.h"
#include "fold.h"
#include "mark.h"
#include "pool.h"
#include "slide.h"

#include <malloc.h>
#include <memory.h>

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

void arena_scratch(Arena *arn){
  arn->cursor = 0;
}

void arena_free(Arena *arn){
  free(arn->mem);
}

void arena_destroy(Arena *arn){
  memset(arn->mem, 0, arn->cap);
  free(arn->mem);
  *arn = (Arena){0};
}

Fold *fold_new(u32 type, u32 len){
  Fold *fold = malloc(sizeof *fold + (type * len));
  fold->next = NULL;
  fold->type = type;
  fold->len = len;
  return fold;
}

void *fold_get(Fold *fold, u32 index){
  if(index < fold->len) return fold->mem + (fold->type * index);
  if(!fold->next) fold->next = fold_new(fold->type, fold->len * 2);
  return fold_get(fold->next, index - fold->len);
}

void fold_reduce(Fold *fold, u32 len){
  if(!fold->next) return;
  if(len <= fold->len){
    fold_free(fold->next);
    fold->next = NULL;
  }
  else fold_reduce(fold->next, len - fold->len);
}

void fold_free(Fold *fold){
  Fold *next = fold->next;
  free(fold);
  if(next) fold_free(next);
}

void fold_destroy(Fold *fold){
  Fold *next = fold->next;
  memset(fold, 0, sizeof *fold + fold->type * fold->len);
  free(fold);
  if(next) fold_destroy(next);
}

struct MarkMarker{
  MarkMarker *next;
  usz size;
};

Mark mark_new(usz size){
  size += sizeof(MarkMarker) - 1;
  size &= ~(sizeof(MarkMarker) - 1);
  Mark mark = {
    .root = malloc(size),
    .ready = mark.root,
    .cap = size
  };
  MarkMarker *marker = mark.root;
  marker->size = size;
  marker->next = NULL;
  return mark;
}

void *mark_alloc(Mark *mark, usz size){
  size += sizeof(MarkMarker) - 1;
  size &= ~(sizeof(MarkMarker) - 1);
  MarkMarker *marker = mark->ready, *src = NULL;
  while(marker){
    if(marker->size == size){
      if(src) src->next = marker->next;
      else mark->ready = marker->next;
      return marker;
    }
    if(marker->size > size){
      if(src){
        src->next = (void*)marker + size;
        src->next->size = marker->size - size;
        src->next->next = marker->next;
      }
      else{
        mark->ready = (void*)marker + size;
        mark->ready->size = marker->size - size;
        mark->ready->next = marker->next;
      }
      return marker;
    }
    src = marker;
    marker = marker->next;
  }
  return NULL;
}

void mark_quickPop(Mark *mark, void *mem, usz size){
  MarkMarker *marker = mark->ready;
  mark->ready = mem;
  mark->ready->size = size;
  mark->ready->next = marker;
}

static void mark_localDefrag(MarkMarker *marker){
recursive:
  if(!marker->next) return;
  if((u8*)marker + marker->size == (u8*)marker->next){
    marker->size += marker->next->size;
    marker->next = marker->next->next;
    goto recursive;
  }
}

void mark_pop(Mark *mark, void *mem, usz size){
  size += sizeof(MarkMarker) - 1;
  size &= ~(sizeof(MarkMarker) - 1);
  MarkMarker *marker = mark->ready, *cast = mem;
  if(!marker || cast < marker){
    cast->next = marker;
    cast->size = size;
    mark->ready = cast;
    mark_localDefrag(mark->ready);
    return;
  }
  while(marker->next && cast > marker->next) marker = marker->next;
  cast->next = marker->next;
  cast->size = size;
  marker->next = cast;
  mark_localDefrag(marker);
}

void mark_defragment(Mark *mark){
  MarkMarker *pivot = NULL, *node;
  while(mark->ready){
    node = mark->ready;
    mark->ready = node->next;
    if(!pivot || node < pivot){
      node->next = pivot;
      pivot = node;
    }
    else{
      MarkMarker *walk = pivot;
      while (walk->next && walk->next < node)
        walk = walk->next;
      node->next = walk->next;
      walk->next = node;
    }
  }
  mark->ready = pivot;
  while(pivot && pivot->next){
    node = pivot->next;
    if((void*)pivot + pivot->size == (void*)node){
      pivot->size += node->size;
      pivot->next = node->next;
    }
    else pivot = pivot->next;
  }
}

void mark_free(Mark *mark){
  free(mark->root);
}

void mark_destroy(Mark *mark){
  memset(mark->root, 0, mark->cap);
  free(mark->root);
  *mark = (Mark){0};
}

const usz mark_pageSize = sizeof(MarkMarker);

struct PoolChunk{
  PoolChunk *next;
};

Pool pool_new(u32 chkSize, u32 chkCount){
  chkSize = MEMLLOC_ALIGN(chkSize);
  Pool pool = {
    .root = malloc(1ull * chkCount * chkSize),
    .ready = pool.root,
    .chkSize = chkSize,
    .chkCount = chkCount,
  };
  PoolChunk *init = pool.root;
  for(u32 i = 1; i < chkCount; ++i){
    init->next = (PoolChunk*)((u8*)init + pool.chkSize);
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
  PoolChunk *chk = addr;
  chk->next = pool->ready;
  pool->ready = chk;
}

void pool_free(Pool *pool){
  free(pool->root);
}

void pool_destroy(Pool *pool){
  memset(pool->root, 0, pool->chkSize * pool->chkCount);
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
  isz offset = (PoolChunk*)addr - poolLink->pool.root;
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

int slide_pop(Slide *sld, void *mem, u32 bytes){
  u32 index = mem - sld->mem;
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

void slide_free(Slide *sld){
  free(sld->mem);
}

void slide_destroy(Slide *sld){
  memset(sld->mem, 0, sld->cap);
  free(sld->mem);
  *sld = (Slide){0};
}