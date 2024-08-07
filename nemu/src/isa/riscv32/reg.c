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
#include <stdio.h>
#include <string.h>
#include "local-include/reg.h"

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

vaddr_t *csr_reg(word_t imm) {
    switch (imm) {
    case 0x300: return &(cpu.csr.mstatus);
    case 0x305: return &(cpu.csr.mtvec);
    case 0x340: return &(cpu.csr.mscratch);
    case 0x341: return &(cpu.csr.mepc);
    case 0x342: return &(cpu.csr.mcause);
    case 0x180: return &(cpu.csr.satp); // in S-mode
    default: 
        panic("unkwon csr.\n"
              "now support mstauts, mtvec, mepc, mcause satp\n");
    } 
}


