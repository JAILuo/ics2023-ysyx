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
#include "debug.h"
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
static word_t get_mtval_for_exception(word_t NO, vaddr_t epc, word_t rd) {
    word_t mtval = 0;
    switch (NO) {
        case EXCP_INST_MISALIGNED:
        case EXCP_INST_ACCESS:
        case EXCP_BREAKPOINT:
        case EXCP_LOAD_ADDR_MISALIGNED:
        case EXCP_LOAD_ACCESS:
        case EXCP_STORE_AMO_MISALIGNED:
        case EXCP_STORE_AMO_ACCESS:
        case EXCP_U_CALL:
        case EXCP_INST_PAGE:
        case EXCP_LOAD_PAGE:
        case EXCP_STORE_AMO_PAGE:
            // 对于访问异常，使用故障的指令或地址
            mtval = rd;
            break;
        case EXCP_ILLEGAL_INST:
            mtval = epc;
            break;
        default:
            mtval = 0;
            break;
    }
    return mtval;
}

static void update_status(word_t NO, vaddr_t epc, word_t rd, bool is_delegate_to_s) {
    // 我委托给S模式，要不要把sstatus的同步mstatus？
    //TODO: sync corresponding s-mode/m-mode register 
    if (is_delegate_to_s) {  
        panic("don't support S-mode");
        sstatus_t sstatus_tmp = {.value = csr_read(CSR_SSTATUS)};  

        word_t stval = get_mtval_for_exception(NO, epc, rd);
        csr_write(CSR_MTVAL, stval); // stval
        
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
        
        word_t mtval = get_mtval_for_exception(NO, epc, rd);
        csr_write(CSR_MTVAL, mtval);

        csr_write(CSR_MEPC, epc);
        csr_write(CSR_MCAUSE, NO);

        mstatus_tmp.mpie = mstatus_tmp.mie;  
        mstatus_tmp.mie = 0;  
        mstatus_tmp.mpp = cpu.priv;  
        //printf("in ecall, mpp: %d, now must be M-mode  mcause_tmp: %x\n", cpu.priv, NO);
        csr_write(CSR_MSTATUS, mstatus_tmp.value);

        cpu.priv = PRIV_MODE_M;  
#ifdef CONFIG_ETRACE  
        mcause_t mcause_tmp = {.value = NO};  
        etrace("M", mcause_tmp.intr, mstatus_tmp.mpp, cpu.csr.mepc,
               mcause_tmp.code, mtval);  
#endif  
    }  
}  

static word_t get_trap_pc(word_t xtvec, word_t xcause) {
    word_t base = (xtvec >> 2) << 2;
    word_t mode = (xtvec & 0x1); // bit 1 is reserved, don't care.
    bool is_intr = (xcause >> (sizeof(word_t) * 8 - 1)) == 1;
    word_t cause_no = ((mcause_t)xcause).code;
    //TODO: add log
    return (is_intr && mode == 1) ? (base + (cause_no << 2)) : base;
}

// ECALL
word_t isa_raise_intr(word_t NO, vaddr_t epc, word_t rd) {
    bool is_intr = ((mcause_t)NO).intr;

    bool is_delegate_to_s = cpu.priv <= PRIV_MODE_S && 
                (is_intr ? BIT(csr_read(CSR_MIDELEG), NO) : BIT(csr_read(CSR_MEDELEG), NO));
    //printf("priv:%d  deleg:%d\n", cpu.priv, is_delegate_to_s);

    update_status(NO, epc, rd, is_delegate_to_s);
    //Log("raise, NO: 0x%x epc: 0x%x  cpu.priv: 0x%x", NO, epc, cpu.priv);
    
    mtvec_t mtvec = { .value = csr_read(CSR_MTVEC)};
    word_t trap_pc = get_trap_pc(mtvec.value, NO);

    if (is_delegate_to_s) {
        // TODO: sync base like mtvec
        panic("don't support S-mode");
        return cpu.csr.stvec; 
    } else {
        return trap_pc;
    }
}

void irq_enable(void) {
    mstatus_t mstatus_tmp = {.value = csr_read(CSR_MSTATUS)};
    mstatus_tmp.mie = true;
    csr_write(CSR_MSTATUS, mstatus_tmp.value);
}

void irq_disable(void) {
    mstatus_t mstatus_tmp = {.value = csr_read(CSR_MSTATUS)};
    mstatus_tmp.mie = false;
    csr_write(CSR_MSTATUS, mstatus_tmp.value);
}

#ifdef CONFIG_RV64
#define INTR_BIT (1ULL << 63)
#else
#define INTR_BIT (1ULL << 31)
#endif

#define IRQ_TIMER 0x80000007

word_t isa_query_intr() {
    mie_t mie = {.value = csr_read(CSR_MIE)};
    mip_t mip = {.value = csr_read(CSR_MIP)};

    // 如果MIE和MIP之间没有共同的位被设置，则没有中断
    if (!(mie.value & mip.value)) return INTR_EMPTY;

    const int priority[] = {
        INTR_M_EXTN, INTR_M_SOFT, INTR_M_TIMR,
        INTR_S_EXTN, INTR_S_SOFT, INTR_S_TIMR,
    };
    int nr_irq = ARRLEN(priority);

    const mstatus_t mstatus = {.value = csr_read(CSR_MSTATUS)};

    /**
     * When running in low-privilege mode (U-mode), 
     * the global interrupt enable bit of high-privilege mode (such as M-mode) is 
     * always in the enable state by default, 
     * that is, the actual value of mstatus.mie is ignored.
     */
    bool global_irq_enable = ((cpu.priv == PRIV_MODE_M) && mstatus.mie) ||
                            (cpu.priv < PRIV_MODE_M);

    for (int i = 0; i < nr_irq; i++) {
        int irq = priority[i];

        bool mie_irq = (mie.value & (1 << irq)) != 0;
        bool mip_irq = (mip.value & (1 << irq)) != 0;

        if (global_irq_enable && mie_irq && mip_irq) {
            mip.value &= ~(1 << irq);
            csr_write(CSR_MIP, mip.value);
            //printf("irq: 0x%llx, now cpu.priv: %d\n", irq | INTR_BIT, cpu.priv);
            return irq | INTR_BIT;
        }
    }
    return INTR_EMPTY; // ((word_t) -1)
}
