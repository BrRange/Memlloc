#ifndef MEMLLOC_SLIDE_
#define MEMLLOC_SLIDE_

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

typedef struct Slide{
  void *mem;
  u32 cap;
  u32 left, right;
} Slide;

Slide slide_new(u32 bytes);

void *slide_alloc(Slide *sld, u32 bytes);

/** Limitation: Surrounded memory cannot be popped
 * \return If the memory was popped
*/
int slide_pop(Slide *sld, void *mem, u32 bytes);

void slide_free(Slide *sld);

void slide_destroy(Slide *sld);

#endif