#ifndef MEMLLOC_POOL_
#define MEMLLOC_POOL_

#ifndef RUSTYDEFH
#define RUSTYDEFH

#define arrLen(_arr) (sizeof(_arr) / sizeof*(_arr))
#define deref(_ptr, _type) (*(_type*)(_ptr))

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

typedef struct PoolChunk PoolChunk;

typedef struct Pool{
  PoolChunk *root, *ready;
  u32 chkSize, chkCount;
} Pool;

Pool pool_new(u32 chkSize, u32 chkCount);

void *pool_alloc(Pool *pool);

void pool_pop(Pool *pool, void *addr);

void pool_free(Pool *pool);

void pool_destroy(Pool *pool);

typedef struct PoolLink{
  Pool pool;
  struct PoolLink *link;
} PoolLink;

PoolLink *poolLink_new(u32 chkSize, u32 chkCount);

void *poolLink_alloc(PoolLink *poolLink);

void poolLink_pop(PoolLink *poolLink, void *addr);

void poolLink_free(PoolLink *poolLink);

void poolLink_destroy(PoolLink *poolLink);

#endif