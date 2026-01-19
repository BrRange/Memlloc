#include "memlloc"
#include <malloc.h>
#include <memory.h>

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
  memset(arn->mem, 0, arn->cap);
  free(arn->mem);
  *arn = (Arena){0};
}

static Fold *module_memlloc_fold_new(u32 type, u32 len){
  Fold *fold = malloc(sizeof *fold + (type * len));
  fold->next = NULL;
  fold->type = type;
  fold->len = len;
  return fold;
}

static void *module_memlloc_fold_get(Fold *fold, u32 index){
  if(index < fold->len) return fold->mem + (fold->type * index);
  if(!fold->next) fold->next = module_memlloc_fold_new(fold->type, fold->len * 2);
  return module_memlloc_fold_get(fold->next, index - fold->len);
}

static void module_memlloc_fold_free(Fold *fold){
  Fold *next = fold->next;
  free(fold);
  if(next) module_memlloc_fold_free(next);
}

static void module_memlloc_fold_reduce(Fold *fold, u32 len){
  if(!fold->next) return;
  if(len <= fold->len){
    module_memlloc_fold_free(fold->next);
    fold->next = NULL;
  }
  else module_memlloc_fold_reduce(fold->next, len - fold->len);
}

static void module_memlloc_fold_destroy(Fold *fold){
  Fold *next = fold->next;
  memset(fold, 0, sizeof *fold + fold->type * fold->len);
  free(fold);
  if(next) module_memlloc_fold_destroy(next);
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

static void module_memlloc_mark_quickPop(Mark *mark, void *mem, usz size){
  MarkMarker *marker = mark->ready;
  mark->ready = mem;
  mark->ready->size = size;
  mark->ready->next = marker;
}

static void module_memlloc_mark_localDefrag(MarkMarker *marker){
recursive:
  if(!marker->next) return;
  if((u8*)marker + marker->size == (u8*)marker->next){
    marker->size += marker->next->size;
    marker->next = marker->next->next;
    goto recursive;
  }
}

static void module_memlloc_mark_pop(Mark *mark, void *mem, usz size){
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
  memset(mark->root, 0, mark->cap);
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
  memset(pool->root, 0, pool->chkSize * pool->chkCount);
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
  memset(sld->mem, 0, sld->cap);
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
  .fold = {
    .new = module_memlloc_fold_new,
    .get = module_memlloc_fold_get,
    .reduce = module_memlloc_fold_reduce,
    .free = module_memlloc_fold_free,
    .destroy = module_memlloc_fold_destroy
  },
  .mark = {
    .new = module_memlloc_mark_new,
    .alloc = module_memlloc_mark_alloc,
    .quickPop = module_memlloc_mark_quickPop,
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