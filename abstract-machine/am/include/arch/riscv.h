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

// mcause: interrupt
enum {
    INTR_S_SOFT = 1,
    INTR_M_SOFT = 3,
    INTR_S_TIMR = 5,
    INTR_M_TIMR = 7,
    INTR_S_EXTN = 9,
    INTR_M_EXTN = 11,
    INTR_COUNTER_OVERFLOW = 13,
};

// mcause: exception
enum {
    EXCP_INST_MISALIGNED = 0,
    EXCP_INST_ACCESS = 1,
    EXCP_ILLEGAL_INST = 2,
    EXCP_BREAKPOINT = 3,
    EXCP_LOAD_ADDR_MISALIGNED = 4,
    EXCP_LOAD_ACCESS = 5,
    EXCP_STORE_AMO_MISALIGNED = 6,
    EXCP_STORE_AMO_ACCESS = 7,
    EXCP_U_CALL = 8,
    EXCP_S_CALL = 9,
    EXCP_RESERVED_1 = 10,
    EXCP_M_CALL = 11,
    EXCP_INST_PAGE = 12,
    EXCP_LOAD_PAGE = 13,
    EXCP_RESERVED_2 = 14,
    EXCP_STORE_AMO_PAGE = 15,
    EXCP_RESERVED_3 = 16,
    EXCP_RESERVED_4 = 17,
    EXCP_SOFT_CHECK = 18,
    EXCP_HARD_ERROR = 19,
    // add more...
};

typedef union mstatus_ {
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
    uintptr_t value;
} mstatus_t;

typedef union mcause_ {
    struct {
#ifdef __ISA_RISCV64__
        uintptr_t code : 63;
#else
        uintptr_t code : 31;
#endif
        uintptr_t intr : 1;
    };
    uintptr_t value;
} mcause_t;

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
