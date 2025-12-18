#ifndef MEMLLOC_ARENA_
#define MEMLLOC_ARENA_

#include <stdlib.h>
#include <stdint.h>
#include "rustydef.h"

typedef struct Arena{
  uint8_t *mem;
  uint32_t cursor, cap;
} Arena;

Arena arena_new(uint32_t bytes);

void *arena_alloc(Arena *arn, uint32_t bytes);

void arena_pop(Arena *arn, uint32_t bytes);

void arena_free(Arena *arn);

void arena_destroy(Arena *arn);

#endif