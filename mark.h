#ifndef MEMLLOC_MARK_
#define MEMLLOC_MARK_

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

typedef struct MarkMarker MarkMarker;

typedef struct Mark{
  void *root;
  MarkMarker *ready;
  usz cap;
} Mark;

Mark mark_new(usz size);

void *mark_alloc(Mark *mark, usz size);

void mark_quickPop(Mark *mark, void *mem, usz size);

void mark_pop(Mark *mark, void *mem, usz size);

void mark_defragment(Mark *mark);

void mark_free(Mark *mark);

void mark_destroy(Mark *mark);

const extern usz mark_pageSize;

#endif