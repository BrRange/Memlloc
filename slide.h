#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef MEMLLOC_SLIDE_
#define MEMLLOC_SLIDE_

typedef struct Slide{
  uint8_t *mem;
  uint32_t cap;
  uint32_t left, right;
} Slide;

Slide slide_new(uint32_t bytes);

void *slide_alloc(Slide *sld, uint32_t bytes);

/** Limitation: Surrounded memory cannot be popped */
bool slide_pop(Slide *sld, uint32_t index, uint32_t bytes);

uint32_t slide_getIndex(Slide *sld, void *mem);

void *slide_getMem(Slide *sld, uint32_t index);

void slide_free(Slide *sld);

void slide_destroy(Slide *sld);

#endif

#ifdef IMPLEMENT_

Slide slide_new(uint32_t bytes){
  bytes = bytes % 8 ? 8ull - (bytes % 8ull) + bytes : bytes;
  Slide sld = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return sld;
}

void *slide_alloc(Slide *sld, uint32_t bytes){
  if(sld->left >= bytes){
    sld->left -= bytes;
    return sld->mem + sld->left;
  }
  if(sld->right <= sld->cap - bytes){
    void *alloc = sld->mem + sld->right;
    sld->right += bytes;
    return alloc;
  }
  return NULL;
}

bool slide_pop(Slide *sld, uint32_t index, uint32_t bytes){
  if(index == sld->left){
    sld->left += bytes;
    return true;
  }
  if(index == sld->right - bytes){
    sld->right -= bytes;
    return true;
  }
  return false;
}

uint32_t slide_getIndex(Slide *sld, void *mem){
  uint8_t *byteMem = mem;
  return byteMem - sld->mem;
}

void *slide_getMem(Slide *sld, uint32_t index){
  return sld->mem + index;
}

void slide_free(Slide *sld){
  free(sld->mem);
}

void slide_destroy(Slide *sld){
  free(sld->mem);
  *sld = (Slide){0};
}

#endif