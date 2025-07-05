#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;
static void *last_malloc_addr = NULL;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

// TODO: when in multi-threading mode, we should add mutex lock to malloc/free
void *malloc(size_t size) {
  if (last_malloc_addr == NULL) last_malloc_addr = heap.start;
  /* simple error handling */
  if (size == 0) return NULL;
  /* TODO: arch related, should rebuild in the future*/
  if (((uintptr_t)last_malloc_addr & 0x3) != 0) {
    // align to 4 bytes
    last_malloc_addr = (void *)ROUNDUP((uintptr_t)last_malloc_addr, 4);
  }
  if (last_malloc_addr + size > heap.end) return NULL;
  void *ptr = last_malloc_addr;
  last_malloc_addr += size;
  return ptr;
}

void free(void *ptr) {
}

#endif
