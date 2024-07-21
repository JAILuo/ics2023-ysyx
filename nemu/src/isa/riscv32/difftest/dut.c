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
#include <cpu/difftest.h>
#include <stdbool.h>
#include <stdio.h>
#include "../local-include/reg.h"

#define CHECK_REG(reg, name) { \
    if ((ref_r->reg) != (cpu.reg)) { \
        printf(name ": diff\n" \
               "ref_r->" name ": " FMT_WORD "  dut->" name ": " FMT_WORD "\n", \
               (ref_r->reg), (cpu.reg)); \
        is_same = false; \
    } \
}

#define CHECK_GPR(i) do { \
    if (ref_r->gpr[i] != cpu.gpr[i]) { \
        printf("reg: %s\n" \
               "ref->gpr: %x  dut->gpr: %x\n", \
               regs[i], ref_r->gpr[i], cpu.gpr[i]); \
        is_same = false; \
    } \
} while (0)

#define PRINT_REF_GPR(i) do { \
    if (is_same == false) { \
        if ((ref_r->gpr[i]) >= 0x80000000) { \
            printf("ref-%3s         %#x\n", regs[i], (ref_r->gpr[i])); \
        } else { \
            printf("ref-%3s         %d(d)\n", regs[i], (ref_r->gpr[i])); \
        } \
    } \
} while (0)

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
     int reg_num = ARRLEN(cpu.gpr);
     int i = 0;
     bool is_same = true;

     CHECK_REG(pc, "pc");
     for (i = 0; i < reg_num; i++) {
         CHECK_GPR(i);
     }

     CHECK_REG(csr.mcause, "mcause");
     CHECK_REG(csr.mstatus, "mstatus");
     CHECK_REG(csr.mepc, "mepc");
     CHECK_REG(csr.mtvec, "mtvec");
     for (i = 0; i < reg_num; i++) {
        PRINT_REF_GPR(i);
     }
     return is_same;
}
void isa_difftest_attach() {
}
