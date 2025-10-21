#include <stdlib.h>
#include <stdint.h>

#ifndef MEMLLOC_ARENA_
#define MEMLLOC_ARENA_

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

#ifdef IMPLEMENT_

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

#endif