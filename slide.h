#ifndef MEMLLOC_SLIDE_
#define MEMLLOC_SLIDE_

#include <stdlib.h>
#include <stdint.h>
#include "rustydef.h"

typedef struct Slide{
  uint8_t *mem;
  uint32_t cap;
  uint32_t left, right;
} Slide;

Slide slide_new(uint32_t bytes);

void *slide_alloc(Slide *sld, uint32_t bytes);

/** Limitation: Surrounded memory cannot be popped */
int slide_pop(Slide *sld, uint32_t index, uint32_t bytes);

uint32_t slide_getIndex(Slide *sld, void *mem);

void *slide_getMem(Slide *sld, uint32_t index);

void slide_free(Slide *sld);

/** Frees and zeroes the datatype's memory */
void slide_destroy(Slide *sld);

#endif