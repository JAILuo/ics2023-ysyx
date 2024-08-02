#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <common.h>

#ifndef PGSIZE
#define PGSIZE 4096
#endif

#define PG_ALIGN __attribute((aligned(PGSIZE)))

void* new_page(size_t);
int mm_brk(uintptr_t brk);
 
#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80

#endif
