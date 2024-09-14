#ifndef __RISCV_CSR_H__
#define __RISCV_CSR_H__

#include <common.h>

#define CSR_MSTATUS      0x300
#define CSR_MTVEC        0x305
#define CSR_MSCRATCH     0x340
#define CSR_MEPC         0x341
#define CSR_MCAUSE       0x342
#define CSR_SATP         0x180

/**
 * Structure members are stored sequentially in the order in which they are declared:
 * the first member has the lowest memory address and the last member the highest.
 */
typedef union CsrMstatus_ {
    struct {
        word_t reserved_0 : 1;
        word_t sie : 1;
        word_t reserved_1 : 1;
        word_t mie : 1;
        word_t reserved_2 : 1;
        word_t spie : 1;
        word_t reserved_3 : 1;
        word_t mpie : 1;
        word_t spp : 1;
        word_t reserved_4 : 2;
        word_t mpp : 2;
        word_t fs : 2;
        word_t xs : 2;
        word_t mprv : 1;
        word_t sum : 1;
        word_t mxr : 1;
        word_t tvm : 1;
        word_t tw : 1;
        word_t tsr : 1;
#ifdef CONFIG_RV64
        word_t reserved_5 : 40; // XLEN - 24
#else
        word_t reserved_5 : 8; // XLEN - 24
#endif
        word_t sd : 1;
    };
    word_t packed;
} CsrMstatus_t;

typedef union CsrMcause_ {
    struct {
#ifdef CONFIG_RV64
        word_t code : 63;
#else
        word_t code : 31;
#endif
        word_t intr : 1;
    };
    word_t packed;
} CsrMcause_t;

vaddr_t *csr_reg(word_t imm);
#define CSRs(i) *csr_reg(i)

#endif
