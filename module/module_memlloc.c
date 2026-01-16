#include "memlloc"
#include <malloc.h>

#ifndef MEMLLOC_ALIGN
#define MEMLLOC_ALIGN(x) ((x + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1))
#endif

static Arena module_memlloc_arena_new(u32 bytes){
  bytes = MEMLLOC_ALIGN(bytes);
  Arena arn = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return arn;
}

static void *module_memlloc_arena_alloc(Arena *arn, u32 bytes){
  if(arn->cursor + bytes > arn->cap) return NULL;
  void *mem = arn->mem + arn->cursor;
  arn->cursor += bytes;
  return mem;
}

static void module_memlloc_arena_pop(Arena *arn, u32 bytes){
  if(bytes >= arn->cursor)
    arn->cursor = 0;
  else
    arn->cursor -= bytes;
}

static void module_memlloc_arena_scratch(Arena *arn){
  arn->cursor = 0;
}

static void module_memlloc_arena_free(Arena *arn){
  free(arn->mem);
}

static void module_memlloc_arena_destroy(Arena *arn){
  free(arn->mem);
  *arn = (Arena){0};
}

struct MarkMarker{
  MarkMarker *next;
  usz size;
};

static Mark module_memlloc_mark_new(usz size){
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

static void *module_memlloc_mark_alloc(Mark *mark, usz size){
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

static void module_memlloc_mark_pop(Mark *mark, void *mem, usz size){
  size += sizeof(MarkMarker) - 1;
  size &= ~(sizeof(MarkMarker) - 1);
  MarkMarker *marker = mark->ready;
  mark->ready = mem;
  if(mem + size == (void*)marker){
    size += marker->size;
    marker = marker->next;
  }
  mark->ready->size = size;
  mark->ready->next = marker;
}

static void module_memlloc_mark_defragment(Mark *mark){
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

static void module_memlloc_mark_free(Mark *mark){
  free(mark->root);
}

static void module_memlloc_mark_destroy(Mark *mark){
  free(mark->root);
  *mark = (Mark){0};
}

typedef struct PoolChunk{
  PoolChunk *next;
} PoolChunk;

static Pool module_memlloc_pool_new(u32 chkSize, u32 chkCount){
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

static void *module_memlloc_pool_alloc(Pool *pool){
  if(!pool->ready) return NULL;
  void *ptr = pool->ready;
  pool->ready = pool->ready->next;
  return ptr;
}

static void module_memlloc_pool_pop(Pool *pool, void *addr){
  PoolChunk *chk = addr;
  chk->next = pool->ready;
  pool->ready = chk;
}

static void module_memlloc_pool_free(Pool *pool){
  free(pool->root);
}

static void module_memlloc_pool_destroy(Pool *pool){
  free(pool->root);
  *pool = (Pool){0};
}

static PoolLink *module_memlloc_poolLink_new(u32 chkSize, u32 chkCount){
  PoolLink *poolLink = malloc(sizeof *poolLink);
  poolLink->pool = module_memlloc_pool_new(chkSize, chkCount);
  poolLink->link = NULL;
  return poolLink;
}

static void *module_memlloc_poolLink_alloc(PoolLink *poolLink){
  void *mem = module_memlloc_pool_alloc(&poolLink->pool);
  if(!mem){
    if(!poolLink->link)
      poolLink->link = module_memlloc_poolLink_new(poolLink->pool.chkSize, poolLink->pool.chkCount * 2);
    return module_memlloc_poolLink_alloc(poolLink->link);
  }
  return mem;
}

static void module_memlloc_poolLink_pop(PoolLink *poolLink, void *addr){
  isz offset = (PoolChunk*)addr - poolLink->pool.root;
  if(offset < 0 || offset >= poolLink->pool.chkCount){
    if(poolLink->link) module_memlloc_poolLink_pop(poolLink->link, addr);
  }
  else module_memlloc_pool_pop(&poolLink->pool, addr);
}

static void module_memlloc_poolLink_free(PoolLink *poolLink){
  PoolLink *next = poolLink->link;
  module_memlloc_pool_free(&poolLink->pool);
  free(poolLink);
  if(next) module_memlloc_poolLink_free(next);
}

static void module_memlloc_poolLink_destroy(PoolLink *poolLink){
  PoolLink *next = poolLink->link;
  module_memlloc_pool_destroy(&poolLink->pool);
  poolLink->link = NULL;
  free(poolLink);
  if(next) module_memlloc_poolLink_destroy(next);
}

static Slide module_memlloc_slide_new(u32 bytes){
  bytes = MEMLLOC_ALIGN(bytes);
  Slide sld = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return sld;
}

static void *module_memlloc_slide_alloc(Slide *sld, u32 bytes){
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

static int module_memlloc_slide_pop(Slide *sld, void *mem, u32 bytes){
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

static void module_memlloc_slide_free(Slide *sld){
  free(sld->mem);
}

static void module_memlloc_slide_destroy(Slide *sld){
  free(sld->mem);
  *sld = (Slide){0};
}

const struct module_memlloc memlloc = {
  .arena = {
    .new = module_memlloc_arena_new,
    .alloc = module_memlloc_arena_alloc,
    .pop = module_memlloc_arena_pop,
    .scratch = module_memlloc_arena_scratch,
    .free = module_memlloc_arena_free,
    .destroy = module_memlloc_arena_destroy
  },
  .mark = {
    .new = module_memlloc_mark_new,
    .alloc = module_memlloc_mark_alloc,
    .pop = module_memlloc_mark_pop,
    .defragment = module_memlloc_mark_defragment,
    .free = module_memlloc_mark_free,
    .destroy = module_memlloc_mark_destroy,
    .pageSize = sizeof(MarkMarker)
  },
  .pool = {
    .new = module_memlloc_pool_new,
    .alloc = module_memlloc_pool_alloc,
    .pop = module_memlloc_pool_pop,
    .free = module_memlloc_pool_free,
    .destroy = module_memlloc_pool_destroy
  },
  .poolLink = {
    .new = module_memlloc_poolLink_new,
    .alloc = module_memlloc_poolLink_alloc,
    .pop = module_memlloc_poolLink_pop,
    .free = module_memlloc_poolLink_free,
    .destroy = module_memlloc_poolLink_destroy
  },
  .slide = {
    .new = module_memlloc_slide_new,
    .alloc = module_memlloc_slide_alloc,
    .pop = module_memlloc_slide_pop,
    .free = module_memlloc_slide_free,
    .destroy = module_memlloc_slide_destroy
  }
};