#include <am.h>
#include <nemu.h>
#include <klib.h>
#include <stdint.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

// kernel page table 
bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  printf("segments len: %d\n", LENGTH(segments));
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

#define VA_VPN_1(addr) ((addr >> 22) & 0x000003ff)
#define VA_VPN_0(addr) ((addr >> 12) & 0x000003ff)
#define VA_OFFSET(addr) (addr & 0x00000fff)

#define PA_OFFSET(addr) (addr & 0x00000fff)
#define PA_PPN(addr)    ((addr >> 12) & 0x000fffff)
#define PTE_PPN (0xFFFFF000) // 31 ~ 12

#define PTE_V_fetch(entry) (entry & 0x1)

/*
void map(AddrSpace *as, void *va, void *pa, int prot) { 
  printf("[map start]\n[VA]: %p, [PA]: %p\n", va, pa);

  uintptr_t va_trans = (uintptr_t) va;
  uintptr_t pa_trans = (uintptr_t) pa;

  assert(PA_OFFSET(pa_trans) == 0);
  assert(VA_OFFSET(va_trans) == 0);

  uint32_t ppn = PA_PPN(pa_trans);
  uint32_t vpn_1 = VA_VPN_1(va_trans);
  uint32_t vpn_0 = VA_VPN_0(va_trans);

  //printf("[PA_PPN]: 0x%x, [VA_VPN_1]: 0x%x, [VA_VPN_0]: 0x%x\n", ppn, vpn_1, vpn_0);

  PTE * page_dir_base = (PTE *) as->ptr;
  PTE * page_dir = page_dir_base + vpn_1;
  //printf("[DIR BASE]: %p, [DIR TARGET]: %p\n", page_dir_base, page_dir);

  if (*page_dir == 0) { // empty
    PTE * page_table_base = (PTE *)pgalloc_usr(PGSIZE);
    *page_dir = ((PTE) page_table_base) | PTE_V;

    PTE * page_table_target = page_table_base + vpn_0;
    *page_table_target = (ppn << 12)| PTE_V | PTE_R | PTE_W | PTE_X;

    //printf("[DIR TARGET ITEM]: 0x%x\n", *page_dir);
    //printf("[TABLE BASE]: %p, [TABLE TARGET]: %p\n", page_table_base, page_table_target);
    //printf("[TABLE TARGET ITEM]: 0x%x\n", *page_table_target);
  } else {
    PTE * page_table_base = (PTE *) ((*page_dir) & PTE_PPN);

    PTE * page_table_target = page_table_base + vpn_0;
    *page_table_target = (ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X;

    //printf("[DIR TARGET ITEM]: 0x%x\n", *page_dir);
    //printf("[TABLE BASE]: %p, [TABLE TARGET]: %p\n", page_table_base, page_table_target);
    //printf("[TABLE TARGET ITEM]: 0x%x\n", *page_table_target);
  }

  //printf("[map end]\n\n");
}
*/

// P111
// SXLEN-bit
// [PTE] Sv32: 4byte, Sv39: 8byte
typedef uintptr_t PTE;

void map(AddrSpace *as, void *va, void *pa, int prot) {
    printf("[begin map]\nva: %p -> pa :%p\n", va, pa);

    uintptr_t va_ = (uintptr_t)va;
    uintptr_t pa_ = (uintptr_t)pa;

    uintptr_t vpn_1 = VA_VPN_1(va_); 
    uintptr_t vpn_0 = VA_VPN_0(va_); 
    uintptr_t pa_ppn = PA_PPN(pa_);
    printf("vpn_1: 0x%x  vpn_0: 0x%0x\n", vpn_1, vpn_0);


    PTE *page_dir_base = (PTE *)as->ptr;
    PTE *page_dir = (PTE *)(page_dir_base + vpn_1);
    //printf("[PAGE DIR BASE]: 0x%x  [PAGE DIR]: 0x%x\n", *page_dir_base, *page_dir);
    printf("[PAGE DIR BASE]: %p  [PAGE DIR]: %p\n", page_dir_base, page_dir);


    // What if it's not a second-order one?
    // How to judge?
    // Or, not to distinguish? Allocate space directly?
    // no.. greater than 0x80000000...
    // maybe only need to 

    // The unit of PT is 4KB, just store high 20-bits
    // The unit of PTE is 4byte
    PTE *pt_base_2 = (PTE *)pgalloc_usr(PGSIZE); // 二级页表
    *page_dir = ((PTE) pt_base_2) | PTE_V;

    //printf("pt_base_2:%p\n", pt_base_2);
    pt_base_2 = (PTE *)(*page_dir); // 填充一级页表的内容
    
    //printf("pt_base_2_new:%p\n", pt_base_2);
    PTE *pte_addr_2 = (PTE *)((uintptr_t)pt_base_2 + vpn_0);

    // Think backwards
    // put pv.ppn into pte of the secondary page table
    // and some bits
    *pte_addr_2 = ((pa_ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X);
    assert(PTE_V_fetch(*pte_addr_2) == 1);

    printf("[TABLE BASE]: %p, [TABLE TARGET]: %p\n", pt_base_2, pte_addr_2);
    printf("[TABLE TARGET ITEM]: 0x%x\n", *pte_addr_2);

    printf("[map end]\n\n");
}


Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
    void *stack_end = kstack.end;
    Context *base = (Context *) ((uint8_t *)stack_end - sizeof(Context));
    // just pass the difftest
    base->mstatus = 0x1800;
    base->gpr[2] = (uintptr_t)kstack.end;
    base->mepc = (uintptr_t)entry;
    return base;
}
