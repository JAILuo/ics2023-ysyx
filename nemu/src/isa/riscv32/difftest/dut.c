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

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
    int reg_num = ARRLEN(cpu.gpr);
    int i = 0;
    bool is_equal = true;
    if (ref_r->pc != cpu.pc) {
        printf("ref_pc" FMT_WORD "  dut_pc:" FMT_WORD "\n",
                ref_r->pc, cpu.pc);
        is_equal = false;
    }
    for (i = 0; i < reg_num; i++) {
        if (ref_r->gpr[i] != cpu.gpr[i]) {
            printf("reg: %s\n"
                   "ref->gpr:%u  dut->gpr:%u\n",
                   regs[i], ref_r->gpr[i], cpu.gpr[i]);
            is_equal = false;
        }
    }
    if (is_equal == false) {
        for (i = 0; i < reg_num; i++) {
            if (ref_r->gpr[i] >= 0x80000000) {
                printf("ref-%3s         %#x\n", regs[i], ref_r->gpr[i]);
            } else {
                printf("ref-%3s         %d\n", regs[i], ref_r->gpr[i]);
            }
        }
    }
    return is_equal;
}

void isa_difftest_attach() {
}
