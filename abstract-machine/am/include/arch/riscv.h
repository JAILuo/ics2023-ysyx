#ifndef ARCH_H__
#define ARCH_H__

#include <stdint.h>
#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

struct Context {
  uintptr_t gpr[NR_REGS]; // 32
  uintptr_t mcause, mstatus, mepc;  // 3
  uintptr_t np;
  void *pdir;
};

#define PRIV_MODE_U 0
#define PRIV_MODE_S 1
#define PRIV_MODE_M 3

enum {
  EXCP_INST_UNALIGNED = 0,
  EXCP_INST_ACCESS = 1,
  EXCP_INST = 2,
  EXCP_BREAK = 3,
  EXCP_READ_UNALIGNED = 4,
  EXCP_READ_ACCESS = 5,
  EXCP_STORE_UNALIGNED = 6,
  EXCP_STORE_ACCESS = 7,
  EXCP_U_CALL = 8,
  EXCP_S_CALL = 9,
  EXCP_M_CALL = 11,
  EXCP_INST_PAGE = 12,
  EXCP_READ_PAGE = 13,
  EXCP_STORE_PAGE = 14,
  // EXCP_15 :AMO pagefault
};

typedef union CsrMstatus_ {
  struct {
    uintptr_t reserved_0 : 1;
    uintptr_t sie : 1;
    uintptr_t reserved_1 : 1;
    uintptr_t mie : 1;
    uintptr_t reserved_2 : 1;
    uintptr_t spie : 1;
    uintptr_t reserved_3 : 1;
    uintptr_t mpie : 1;
    uintptr_t spp : 1;
    uintptr_t reserved_4 : 2;
    uintptr_t mpp : 2;
    uintptr_t fs : 2;
    uintptr_t xs : 2;
    uintptr_t mprv : 1;
    uintptr_t sum : 1;
    uintptr_t mxr : 1;
    uintptr_t tvm : 1;
    uintptr_t tw : 1;
    uintptr_t tsr : 1;
#ifdef __ISA_RISCV64__
    uintptr_t reserved_5 : 40; // XLEN - 24
#else
    uintptr_t reserved_5 : 8; // XLEN - 24
#endif
    uintptr_t sd : 1;
  };
  uintptr_t packed;
} CsrMstatus_t;

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[10] // a0
#define GPR3 gpr[11] // a1
#define GPR4 gpr[12] // a2
#define GPRx gpr[10] // a0

#endif
