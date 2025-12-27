#ifndef MEMLLOC_ARENA_
#define MEMLLOC_ARENA_

#include <stdlib.h>
#include <stdint.h>
#include "rustydef.h"

typedef struct Arena{
  void *mem;
  u32 cursor, cap;
} Arena;

Arena arena_new(u32 bytes);

void *arena_alloc(Arena *arn, u32 bytes);

void arena_pop(Arena *arn, u32 bytes);

void arena_free(Arena *arn);

void arena_destroy(Arena *arn);

#endif