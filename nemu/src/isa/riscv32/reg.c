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

#include <isa.h>
#include <cpu/cpu.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "local-include/reg.h"
#include "local-include/csr.h"

#define CSRs(i) *csr_reg(i)

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
    int i = 0;
    /*
    printf("----------\t---------------------------\n");
    printf("|Register|\t|           Value         |\n");
    printf("----------\t---------------------------\n");
    */
    printf("dut_reg:\n");
    for (i = 0;  i < ARRLEN(regs); i++) {
        // General-Purpose Registers
        printf("%-16s0x%08x\t%d\n", regs[i], gpr(i), gpr(i)); 
    }
}

word_t isa_reg_str2val(const char *s, bool *success) {
    // pc and r0-r31
    if (strcmp(s, "pc") == 0 ) {
        *success = true;
        return cpu.pc;
    }

    for (int i = 0; i < ARRLEN(regs); i++) {
        if (strcmp(s, regs[i]) == 0) {
            *success = true;
            return gpr(i);
        } 
    }
    printf("No this reg: %s\n",s);
    *success = false;
    return 0;
}

// opf programming video? 
// the structure like &(p1->p2.p3.p4) has a minimum number of instructions?
static vaddr_t *csr_reg(word_t imm) {
    switch (imm) {
        // M-mode CSR
    case CSR_MSTATUS:       return &(cpu.csr.mstatus);
    case CSR_MEDELEG:       return &(cpu.csr.medeleg);
    case CSR_MIDELEG:       return &(cpu.csr.mideleg);
    case CSR_MIE:           return &(cpu.csr.mie);
    case CSR_MTVEC:         return &(cpu.csr.mtvec);
    case CSR_MSCRATCH:      return &(cpu.csr.mscratch);
    case CSR_MEPC:          return &(cpu.csr.mepc);
    case CSR_MCAUSE:        return &(cpu.csr.mcause);
    case CSR_MTVAL:         return &(cpu.csr.mtval);
    case CSR_MIP:           return &(cpu.csr.mip);
    case CSR_MCYCLE:        return &(cpu.csr.mcycle);
    //case CSR_MINSTRET:      return &(cpu.csr.minstret); 

    // S-mode CSR
    case CSR_SSTATUS:       return &(cpu.csr.sstatus);
    case CSR_STVEC:         return &(cpu.csr.stvec);
    case CSR_SSCRATCH:      return &(cpu.csr.sscratch);
    case CSR_SEPC:          return &(cpu.csr.sepc);
    case CSR_SCAUSE:        return &(cpu.csr.scause);
    case CSR_SATP:          return &(cpu.csr.satp);

    default: panic("unkwon csr.");
    } 
}

// TODO: Add access to CSR
// how to ensure S/U-mode can't access the M-Mode csr and inst?
word_t csr_read(uint16_t csr_num) {
    return CSRs(csr_num);
}

void csr_write(uint16_t csr_num, word_t data) {
    CSRs(csr_num) = data;
}


