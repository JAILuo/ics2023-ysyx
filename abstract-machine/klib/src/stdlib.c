#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdbool.h>

// riscv32-nemu use: maybe i can use marco the test.
//static char *start_addr = NULL;
//static bool is_init = false;

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

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

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
    static void *last_addr = NULL;
    if (!last_addr) {
        last_addr = (void*)ROUNDUP(heap.start, 4);
    }
    size_t size_adj = size & 0xF ? (size & ~(size_t)0xF) + 0x10 : size;
    void *old = last_addr;
    last_addr = (uint8_t *)last_addr + size_adj;
    return old;
#endif
    return NULL;
}

void free(void *ptr) {
}

#endif
