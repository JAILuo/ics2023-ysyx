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

#include "common.h"
#include "isa-def.h"
#include <isa.h>
#include <../local-include/reg.h>
#include <../local-include/csr.h>
#include <stdbool.h>

#define IRQ_TIMER 0x80000007  // for riscv32

// ECALL
void etrace_log(CsrMstatus_t mstatus_tmp, CsrMcause_t mcause_tmp);
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
    CsrMstatus_t mstatus_tmp = {.packed = CSRs(CSR_MSTATUS)};
    
    CSRs(CSR_MEPC) = epc;
    CSRs(CSR_MCAUSE) = NO;

    // TODO: 把发生异常时的虚拟地址更新到mtval寄存器中
    
    // 把异常发生前的 MIE 字段 保存到 MPIE 字段 
    mstatus_tmp.mpie = mstatus_tmp.mie;
    // 关闭本地中断 MIE = 0
    mstatus_tmp.mie = 0;
    // 保存处理器模式 MPP bit[9:8] 0b11 M mode  
    mstatus_tmp.mpp = cpu.priv;
    CSRs(CSR_MSTATUS) = mstatus_tmp.packed;
    // 设置处理器模式为 M 模式
    cpu.priv = PRIV_MODE_M;

    // TODO: add more mcause, mepc trace!!
#ifdef CONFIG_ETRACE
    CsrMcause_t mcause_tmp = {.packed = NO};
    etrace_log(mstatus_tmp, mcause_tmp);
#endif
    return cpu.csr.mtvec;
}

void etrace_log(CsrMstatus_t mstatus_tmp, CsrMcause_t mcause_tmp) {
    if (mcause_tmp.intr) {
        // TODO: need to add X-Macre to show the exception name.
        printf("[etrace begin]\n"
               "    [interrupt]\n"
               "    mstatus.mie: %u, mstatus.mpie: %u\n"
               "    mepc: " FMT_WORD ", mcause.code: " FMT_WORD "\n"
               "[etrace_log end]\n",
            mstatus_tmp.mie, mstatus_tmp.mpie, cpu.csr.mepc, mcause_tmp.code);
    } else {
        printf("[etrace begin]\n"
               "    [exception]\n"
               "    mstatus.mie: %u, mstatus.mpie: %u\n"
               "    mepc: " FMT_WORD ", mcause.code: " FMT_WORD "\n"
               "[etrace_log end]\n",
            mstatus_tmp.mie, mstatus_tmp.mpie, cpu.csr.mepc, mcause_tmp.code);
    }
}
// TODO: Some apps triggered access exceptions?

word_t isa_query_intr() {
    const CsrMstatus_t mstatus_tmp = {.packed = CSRs(CSR_MSTATUS)};
    // MIE = 1, enable interrupt
    if (cpu.INTR == true && mstatus_tmp.mie != 0) {
        cpu.INTR = false;
        // TODO: real ISA need to what? when coming interrupt
        return IRQ_TIMER;
    }
    return INTR_EMPTY; // ((word_t) -1)
}
