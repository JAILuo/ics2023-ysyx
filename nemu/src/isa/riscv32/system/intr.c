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
#include <../local-include/reg.h>
#include <stdio.h>

// ECALL
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
    cpu.csr.mepc = epc;
    cpu.csr.mcause = NO;

    cpu.csr.mstatus &= ~(1 << 7);

    // 把异常发生前的 MIE 字段 保存到 MPIE 字段 
    cpu.csr.mstatus |= ((cpu.csr.mstatus & (1 << 3)) << 4);
    // 关闭本地中断 MIE = 0
    cpu.csr.mstatus &= ~(1 << 3);
    // 保存处理器模式 MPP bit[9:8] 0b11 M mode
    cpu.csr.mstatus |= ((1 << 11) + (1 << 12));

    return cpu.csr.mtvec;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
