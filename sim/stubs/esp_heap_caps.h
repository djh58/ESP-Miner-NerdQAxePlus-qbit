#pragma once
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM (1 << 10)
#define MALLOC_CAP_8BIT (1 << 2)
#define MALLOC_CAP_DMA (1 << 3)
#define MALLOC_CAP_INTERNAL (1 << 11)
static inline void *heap_caps_malloc(size_t size, unsigned caps) { (void) caps; return malloc(size); }
static inline void *heap_caps_calloc(size_t n, size_t size, unsigned caps) { (void) caps; return calloc(n, size); }
static inline void *heap_caps_realloc(void *p, size_t size, unsigned caps) { (void) caps; return realloc(p, size); }
static inline void heap_caps_free(void *p) { free(p); }
