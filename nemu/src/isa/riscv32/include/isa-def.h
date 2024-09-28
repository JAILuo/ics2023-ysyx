/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#define PRIV_MODE_U 0
#define PRIV_MODE_S 1
#define PRIV_MODE_M 3

// m/scause: interrupt
enum {
    INTR_S_SOFT = 1,
    INTR_M_SOFT = 3,
    INTR_S_TIMR = 5,
    INTR_M_TIMR = 7,
    INTR_S_EXTN = 9,
    INTR_M_EXTN = 11,
    INTR_COUNTER_OVERFLOW = 13,
};

// m/scause: exception
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

typedef struct {
    word_t mcause;
    word_t mstatus;
    word_t mepc;
    word_t mtvec;
    word_t mscratch;

    word_t mtval;
    word_t mcycle;
    word_t medeleg;
    word_t mideleg;
    //word_t minstret;
    word_t mip;
    word_t mie;

    word_t satp;
    word_t sepc;
    word_t stvec;
    word_t sstatus;
    word_t scause;
    word_t sscratch;
    // add more...
} riscv_CPU_csr;


typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  riscv_CPU_csr csr;
  int priv;
  bool INTR;
  
  //RV32A
  bool lr_valid;
  word_t reserved_addr;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);


#define isa_mmu_check(vaddr, len, type) \
    ((((cpu.csr.satp & 0x80000000) >> 31) == 1) ? MMU_TRANSLATE : MMU_DIRECT)


#endif
