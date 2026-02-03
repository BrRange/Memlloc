#ifndef MEMLLOC_FOLD_
#define MEMLLOC_FOLD_

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

typedef struct Fold{
  struct Fold *next;
  u32 type, len;
  u8 mem[];
} Fold;

Fold *fold_new(u32 type, u32 len);

void *fold_get(Fold *fold, u32 index);

void fold_reduce(Fold *fold, u32 len);

void fold_free(Fold *fold);

void fold_destroy(Fold *fold);

#endif