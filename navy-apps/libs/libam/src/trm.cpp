#include <am.h>

Area heap;

//#define __riscv__
#define __X86__

#if defined(__X86__)
# define nemu_trap(code) asm volatile ("int3" : :"a"(code))
#elif defined(__MIPS32__)                                                                                             
# define nemu_trap(code) asm volatile ("move $v0, %0; sdbbp" : :"r"(code))
#elif defined(__riscv__)
# define nemu_trap(code) asm volatile("mv a0, %0; ebreak" : :"r"(code))
#elif defined(__LOONGARCH32R__)
# define nemu_trap(code) asm volatile("move $a0, %0; break 0" : :"r"(code))
#elif
# error unsupported ISA __ISA__
#endif


void putch(char ch) {
    putchar(ch);
}

void halt(int code) {
    nemu_trap(code);
    
    // should not reach here
    while (1);
}
