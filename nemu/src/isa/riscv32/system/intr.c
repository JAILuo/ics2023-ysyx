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

#ifdef CONFIG_ETRACE
const char *priv_name[] = {
    "U-mode", // 0
    "S-mode", // 1
    "null",   // 2
    "M-mode"  // 3
};
static inline int check_priv_name(int pp) {
  IFDEF(CONFIG_RT_CHECK, assert(pp >= 0 && pp < 4));
  return pp;
}
#define PRIV_NAME(pp) (priv_name[check_priv_name(pp)])

static void etrace(const char* mode, bool is_interrupt, int pp, 
                   word_t epc, unsigned int code, word_t tval) {  
    printf("[etrace begin] " "\33[1;36m%s-mode\33[0m CSR\n"  
           "    [%s]\n"
           "    previous_privilege: \33[1;36m%s\33[0m\n"  
           "    epc: " FMT_WORD ", cause.code: " FMT_WORD ", tval:" FMT_WORD "\n"
           "[etrace end]\n",  
           mode, is_interrupt ? "interrupt" : "exception", 
           PRIV_NAME(pp), epc, code, tval);  
}  
#endif

//TODO: how to add the addr of exception.
static word_t get_mtval_for_exception(word_t NO, vaddr_t epc) {
    word_t mtval = 0;
    switch (NO) {
        case EXCP_INST_MISALIGNED:
        case EXCP_INST_ACCESS:
        case EXCP_LOAD_ADDR_MISALIGNED:
        case EXCP_LOAD_ACCESS:
        case EXCP_STORE_AMO_MISALIGNED:
        case EXCP_STORE_AMO_ACCESS:
            // 对于访问异常，使用故障的指令或地址
            mtval = epc;
            break;
        case EXCP_ILLEGAL_INST:
            // 对于非法指令异常，获取故障指令的编码
            mtval = epc;
            break;
        case EXCP_BREAKPOINT:
            // 对于断点异常，可能需要特殊的处理
            mtval = epc;
            break;
        default:
            // 对于不需要更新 mtval 的异常，保持 mtval 为 0
            break;
    }
    return mtval;
}

static void update_status(word_t NO, vaddr_t epc, bool is_delegate_to_s) {
    // 我委托给S模式，要不要把sstatus的同步mstatus？
    if (is_delegate_to_s) {  
        sstatus_t sstatus_tmp = {.value = csr_read(CSR_SSTATUS)};  

        word_t stval = get_mtval_for_exception(NO, epc);
        csr_write(CSR_MTVAL, stval);
        
        csr_write(CSR_SEPC, epc);
        csr_write(CSR_SCAUSE, NO);
        sstatus_tmp.spie = sstatus_tmp.sie;  
        sstatus_tmp.sie = 0;  
        sstatus_tmp.spp = cpu.priv;  
        csr_write(CSR_SSTATUS, sstatus_tmp.value);

        cpu.priv = PRIV_MODE_S;  
#ifdef CONFIG_ETRACE  
        scause_t scause_tmp = {.value = NO};  
        etrace("S", scause_tmp.intr, sstatus_tmp.spp, cpu.csr.sepc, 
               scause_tmp.code, stval);  
#endif  
    } else {  
        mstatus_t mstatus_tmp = {.value = csr_read(CSR_MSTATUS)};  
        
        word_t mtval = get_mtval_for_exception(NO, epc);
        csr_write(CSR_MTVAL, mtval);

        csr_write(CSR_MEPC, epc);
        csr_write(CSR_MCAUSE, NO);
        mstatus_tmp.mpie = mstatus_tmp.mie;  
        mstatus_tmp.mie = 0;  
        mstatus_tmp.mpp = cpu.priv;  
        csr_write(CSR_MSTATUS, mstatus_tmp.value);

        cpu.priv = PRIV_MODE_M;  
#ifdef CONFIG_ETRACE  
        mcause_t mcause_tmp = {.value = NO};  
        etrace("M", mcause_tmp.intr, mstatus_tmp.mpp, cpu.csr.mepc,
               mcause_tmp.code, mtval);  
#endif  
    }  
}  

// ECALL
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
    bool is_intr = ((mcause_t)NO).intr;
    //printf("is_intr:%d\n", is_intr);

    bool is_delegate_to_s = cpu.priv <= PRIV_MODE_S && 
                (is_intr ? BIT(csr_read(CSR_MIDELEG), NO) : BIT(csr_read(CSR_MEDELEG), NO));
    //printf("priv:%d  deleg:%d\n", cpu.priv, is_delegate_to_s);

    update_status(NO, epc, is_delegate_to_s);

    if (is_delegate_to_s) {
        return cpu.csr.stvec;
    } else {
        return cpu.csr.mtvec; 
    }
}


#ifdef CONFIG_RV64
#define INTR_BIT (1ULL << 63)
#else
#define INTR_BIT (1ULL << 31)
#endif

//  TODO: need to add mie, mip, m/stvec
word_t isa_query_intr() {
    const int priority[] = {
        INTR_M_EXTN, INTR_M_SOFT, INTR_M_TIMR,
        INTR_S_EXTN, INTR_S_SOFT, INTR_S_TIMR,
    };
    int nr_irq = ARRLEN(priority);

    //mideleg_t mideleg_tmp = (mideleg_t)csr_read(CSR_MIDELEG);
    const mstatus_t mstatus_tmp = {.value = csr_read(CSR_MSTATUS)};
    //printf("mie:%d\n", mstatus_tmp.mie);

    bool global_irq_enable = mstatus_tmp.mie;
    // bool enable_irq = global_irq_enable && mie_irq && concrete_mip_ieq;
    //printf("global_irq_enable: %d\n", global_irq_enable);

    if (cpu.INTR == true && global_irq_enable == true) {
        cpu.INTR = false;
        for (int i = 0; i < nr_irq; i++) {
            // TODO: the problem now is how to distinguish 3 M-intr
            // now the code I wrote is be bound to trigger intr...
            // we need a func to judge which intr
            int irq = priority[i];
            //printf("irq_mum:%llx\n", irq | INTR_BIT);

            mie_t mie = {.value = csr_read(CSR_MIE)};
            mip_t mip = {.value = csr_read(CSR_MIP)};
            //printf("mie: " FMT_WORD "\nmip: " FMT_WORD "\n", mie, mip);
            bool mie_irq = (mie.value & (1 << irq)) != 0;
            bool mip_irq = (mip.value & (1 << irq)) != 0;
            //printf("mie_irq: %d\n", mie_irq);
            //printf("mip_irq: %d\n", mip_irq);
            
            if (mie_irq && mip_irq) {
                return irq | INTR_BIT;
            }

            // bool deleg = (mideleg_tmp.value & (1 << irq)) != 0;
            // bool global_irq_enable = deleg ?
            //     ((cpu.priv == PRIV_MODE_S) && mstatus_tmp.sie) || (cpu.priv < PRIV_MODE_S) :
            //     ((cpu.priv == PRIV_MODE_M) && mstatus_tmp.mie) || (cpu.priv < PRIV_MODE_M);
            // if (mstatus_tmp.mie) {
            // 
            // }
            // //printf("enable:%d\n", global_irq_enable);
            //                             
            // //printf("priority:%d\n", irq);
            // //printf("irq_mum:%llx\n", irq | INTR_BIT);
            // if (global_irq_enable) return irq | INTR_BIT;
        }
    }

    // const mstatus_t mstatus_tmp = {.value = csr_read(CSR_MSTATUS)};
    // if (cpu.INTR == true && mstatus_tmp.mie != 0) {
    //     cpu.INTR = false;
    //     // TODO: need to add more interrupt, not only INTR_M_TIMR
    //     mcause_t mcause_tmp = {.intr = true, .code = INTR_M_TIMR};
    //     return mcause_tmp.value;
    // }
    return INTR_EMPTY; // ((word_t) -1)
}
