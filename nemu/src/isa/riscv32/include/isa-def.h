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

typedef struct {
    word_t mcause;
    word_t mstatus;
    word_t mepc;
    word_t mtvec;
    word_t mscratch;
    word_t satp;
    // add more...
} riscv_CPU_csr;

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  riscv_CPU_csr csr;
  int priv;
  bool INTR;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);


#define isa_mmu_check(vaddr, len, type) ((((cpu.csr.satp & 0x80000000) >> 31) == 1) ? MMU_TRANSLATE : MMU_DIRECT)


#endif
