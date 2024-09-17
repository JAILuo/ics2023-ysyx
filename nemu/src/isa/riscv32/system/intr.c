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
#include "macro.h"
#include <isa.h>
#include <../local-include/reg.h>
#include <../local-include/csr.h>
#include <stdbool.h>

// 合并的 etrace 函数  
static void etrace(const char* mode, bool is_interrupt, int pp, 
                   word_t epc, unsigned int code) {  
    printf("[etrace begin] " "\33[1;36m%s-mode\33[0m CSR:\n"  
           "    [%s]\n"
           "    previous_privilege: %d\n"  
           "    epc: " FMT_WORD ", cause.code: " FMT_WORD "\n"  
           "[etrace end]\n",  
           mode, is_interrupt ? "interrupt" : "exception", pp, epc, code);  
}  

static void update_status(word_t NO, vaddr_t epc, bool is_delegate_to_s) {  
    if (is_delegate_to_s) {  
        sstatus_t sstatus_tmp = {.value = CSRs(CSR_SSTATUS)};  
        
        CSRs(CSR_SEPC) = epc;  
        CSRs(CSR_SCAUSE) = NO;  
        sstatus_tmp.spie = sstatus_tmp.sie;  
        sstatus_tmp.sie = 0;  
        sstatus_tmp.spp = cpu.priv;  
        CSRs(CSR_SSTATUS) = sstatus_tmp.value;  

        cpu.priv = PRIV_MODE_S;  
#ifdef CONFIG_ETRACE  
        scause_t scause_tmp = {.value = NO};  
        etrace("S", scause_tmp.intr, sstatus_tmp.spp, cpu.csr.sepc, scause_tmp.code);  
#endif  
    } else {  
        mstatus_t mstatus_tmp = {.value = CSRs(CSR_MSTATUS)};  
        
        CSRs(CSR_MEPC) = epc;  
        CSRs(CSR_MCAUSE) = NO;  
        //TODO: add mtval to store addr?
        mstatus_tmp.mpie = mstatus_tmp.mie;  
        mstatus_tmp.mie = 0;  
        mstatus_tmp.mpp = cpu.priv;  
        CSRs(CSR_MSTATUS) = mstatus_tmp.value;  

        cpu.priv = PRIV_MODE_M;  
#ifdef CONFIG_ETRACE  
        mcause_t mcause_tmp = {.value = NO};  
        etrace("M", mcause_tmp.intr, mstatus_tmp.mpp, cpu.csr.mepc, mcause_tmp.code);  
#endif  
    }  
}  

// ECALL
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
    bool is_intr = ((mcause_t)NO).intr;

    bool is_delegate_to_s = cpu.priv <= PRIV_MODE_S && 
                (is_intr ? BIT(CSRs(CSR_MIDELEG), NO) : BIT(CSRs(CSR_MEDELEG), NO));

    update_status(NO, epc, is_delegate_to_s);

    if (is_delegate_to_s) {
        return cpu.csr.stvec;
    } else {
        return cpu.csr.mtvec; 
    }
}

word_t isa_query_intr() {
    const mstatus_t mstatus_tmp = {.value = CSRs(CSR_MSTATUS)};
    if (cpu.INTR == true && mstatus_tmp.mie != 0) {
        cpu.INTR = false;
        // TODO: need to add more interrupt, not only INTR_M_TIMR
        mcause_t mcause_tmp = {.intr = true, .code = INTR_M_TIMR};
        return mcause_tmp.value;
    }
    return INTR_EMPTY; // ((word_t) -1)
}
