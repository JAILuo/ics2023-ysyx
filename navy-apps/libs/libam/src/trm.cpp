#include <am.h>

Area heap;

#define nemu_trap(code) asm volatile("mv a0, %0; ebreak" : :"r"(code))

void putch(char ch) {
    putchar(ch);
}

void halt(int code) {
      nemu_trap(code);

  // should not reach here
  while (1);
}
