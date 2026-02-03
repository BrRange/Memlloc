#ifndef MEMLLOC_ARENA_
#define MEMLLOC_ARENA_

#ifndef RUSTY_DEFINITION_
#define RUSTY_DEFINITION_

#define arrLen(_arr) (sizeof(_arr) / sizeof*(_arr))
#define deref(_ptr, _type) (*(_type*)(_ptr))
#define pointer(_type...) typeof(typeof(_type)*)
#define defer(_freeFn) __attribute__((cleanup(_freeFn)))

#include <stdint.h>
#include <stdbool.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isz;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usz;
typedef float f32;
typedef double f64;

#endif

typedef struct Arena{
  void *mem;
  u32 cursor, cap;
} Arena;

Arena arena_new(u32 bytes);

void *arena_alloc(Arena *arn, u32 bytes);

void arena_pop(Arena *arn, u32 bytes);

void arena_scratch(Arena *arn);

void arena_free(Arena *arn);

void arena_destroy(Arena *arn);

#endif