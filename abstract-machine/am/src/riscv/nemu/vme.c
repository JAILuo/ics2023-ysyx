#include <am.h>
#include <nemu.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <stdbool.h>
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
  // __riscv_xlen are predefined macros for the RISCV GCC toolchain
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
  // Use only the lower 20 bits of SATP, align 4byte
}

// kernel page table 
bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
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


// P111
// SXLEN-bit
// [PTE] Sv32: 4byte, Sv39: 8byte


// If it's not a second-order one?
// How to judge?
// Or, not to distinguish? Allocate space directly?
// no.. greater than 0x80000000...
// maybe only need to 
void map(AddrSpace *as, void *va, void *pa, int prot) {
    //printf("[begin map]\nva: %p -> pa :%p\n", va, pa);

    uintptr_t va_ = (uintptr_t)va;
    uintptr_t pa_ = (uintptr_t)pa;
    assert(PA_OFFSET(pa_) == VA_OFFSET(va_));
    
    // vme_init Maps the space starting with 0x80000000 in units of 4KB.
    // That is, each time add 0x1000: 1000, 2000, for vpn_0,
    // it is to add 1 each time 
    uintptr_t vpn_1 = VA_VPN_1(va_); 
    uintptr_t vpn_0 = VA_VPN_0(va_); 
    //printf("vpn_1: 0x%x  vpn_0: 0x%0x\n", vpn_1, vpn_0);

    PTE *page_dir_base = (PTE *)as->ptr;
    PTE *page_dir = (PTE *)(page_dir_base + vpn_1); 
    // Because of the pointer arithmetic, it is 4bytes at a time
    //printf("[PAGE DIR BASE]: 0x%x  [PAGE DIR]: 0x%x\n", *page_dir_base, *page_dir);
    //printf("[PAGE DIR BASE]: %p  [PAGE DIR]: %p\n", page_dir_base, page_dir);


    // The unit of PT is 4KB, just store high 20-bits
    // The unit of PTE is 4byte
    //static int i = 1;
    PTE *pt_2_base = NULL;
    if (*page_dir == 0) {
        pt_2_base = (PTE *)pgalloc_usr(PGSIZE);
        *page_dir = ((PTE)pt_2_base) | PTE_V; // Populate the first-level pte
        //printf("========%d\n", i);
    } else {
        pt_2_base = (PTE *)((*page_dir) & PTE_PPN);
        //i++;
    }

    // Adding a vpn_0 is adding 4 bytes, because it is a pointer operation.
    // A total of 1024 times, a total of 4KB.
    // Each time, a 4-byte PTE is filled, for a total of 1024 PTEs.
    // Once the 1024 PTEs are filled, a new page table is assigned 
    // (i.e. == 0 above)

    uintptr_t pa_ppn = PA_PPN(pa_);
    PTE *pte_2_addr = pt_2_base + vpn_0;
    *pte_2_addr = (pa_ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X; // Populate the second-level pte
    //printf("*pte_2_addr: 0x%x\n", *pte_2_addr);

    //printf("[pt_2_base]: %p, [pte_2_addr]: %p\n", pt_2_base, pte_2_addr);
    //printf("[*pte_2_addr]: 0x%x\n", *pte_2_addr);
    //printf("[map end]\n\n");
}


Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
    void *stack_end = kstack.end;
    Context *base = (Context *) ((uint8_t *)stack_end - sizeof(Context));
    // just pass the difftest
    //base->mstatus = 0x1800; // MPP bit[12:11] 0b11 = 3
    const mstatus_t mstatus_tmp = {
        .mpie = 1,
        .mie = 0,
        .sum = 1, // read note and manual
        .mxr = 1, // about S-mode, OS will do this, design processor core don't care?
        .mpp = PRIV_MODE_U,
    };
  printf("iset:%d\n", ienabled());
    // notice the MPIE will be restored to the MIE in nemu
    //base->mstatus |= (1 << 7); MPIE = 1;
    base->mstatus = mstatus_tmp.value;
    base->pdir = as->ptr;
    base->np = PRIV_MODE_U;
    base->gpr[2] = (uintptr_t)kstack.end;
    base->mepc = (uintptr_t)entry;
    return base;
}
