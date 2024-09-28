#ifndef __RISCV_CSR_H__
#define __RISCV_CSR_H__

#include <common.h>

#define CSR_MSTATUS     0x300
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344
#define CSR_MCYCLE      0xb00
#define CSR_MINSTRET    0xb02

#define CSR_SSTATUS     0x100
#define CSR_STVEC       0x105
#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_SATP        0x180


/**
 * Structure members are stored sequentially in the order in which they are declared:
 * the first member has the lowest memory address and the last member the highest.
 */
typedef union mstatus_ {
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
    word_t value;
} mstatus_t;

typedef union sstatus_ {
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
    word_t value;
} sstatus_t;

typedef union mcause_ {
    struct {
#ifdef CONFIG_RV64
        word_t code : 63;
#else
        word_t code : 31;
#endif
        word_t intr : 1;
    };
    word_t value;
} mcause_t;

typedef union scause_ {
    struct {
#ifdef CONFIG_RV64
        word_t code : 63;
#else
        word_t code : 31;
#endif
        word_t intr : 1;
    };
    word_t value;
} scause_t;

typedef union mideleg_ {
    struct {
#ifdef CONFIG_RV64
        word_t code : 63;
#else
        word_t code : 31;
#endif
        word_t intr : 1;
    };
    word_t value;
} mideleg_t;

typedef union medeleg_ {
    struct {
#ifdef CONFIG_RV64
        word_t code : 63;
#else
        word_t code : 31;
#endif
        word_t intr : 1;
    };
    word_t value;
} medeleg_t;

typedef union mip_ {
    struct {
        word_t reserved_0 : 1;
        word_t ssip : 1;
        word_t reserved_1 : 1;
        word_t msip : 1;
        word_t reserved_2 : 1;
        word_t stip : 1;
        word_t reserved_3 : 1;
        word_t mtip : 1;
        word_t reserved_4 : 1;
        word_t seip : 1;
        word_t reserved_5 : 1;
        word_t meip : 1;
#ifdef CONFIG_RV64
        word_t code : 52;
#else
        word_t code : 20;
#endif
    };
    word_t value;
} mip_t;

typedef union mie_ {
    struct {
        word_t reserved_0 : 1;
        word_t ssie : 1;
        word_t reserved_1 : 1;
        word_t msie : 1;
        word_t reserved_2 : 1;
        word_t stie : 1;
        word_t reserved_3 : 1;
        word_t mtie : 1;
        word_t reserved_4 : 1;
        word_t seie : 1;
        word_t reserved_5 : 1;
        word_t meie : 1;
#ifdef CONFIG_RV64
        word_t code : 52;
#else
        word_t code : 20;
#endif
    };
    word_t value;
} mie_t;

word_t csr_read(uint16_t csr_num);
void csr_write(uint16_t csr_num, word_t data);

#endif
