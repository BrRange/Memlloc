#ifndef MEMLLOC_SLIDE_
#define MEMLLOC_SLIDE_

#include <stdlib.h>
#include <stdint.h>
#include "rustydef.h"

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
int slide_pop(Slide *sld, u32 index, u32 bytes);

u32 slide_getIndex(Slide *sld, void *mem);

void *slide_getMem(Slide *sld, u32 index);

void slide_free(Slide *sld);

/** Frees and zeroes the datatype's memory */
void slide_destroy(Slide *sld);

#endif