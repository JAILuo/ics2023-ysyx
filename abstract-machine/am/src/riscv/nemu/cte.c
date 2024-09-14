#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <stdint.h>

static Context* (*user_handler)(Event, Context*) = NULL;

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

Context* __am_irq_handle(Context *c) {
    __am_get_cur_as(c);

    if (user_handler) {
        //printf("in __am_irq_handle,c:%p c->sp:%p\n", c, c->gpr[2]);
        //printf("mcause: %x\n", c->mcause);
        Event ev = {0};
        // Whether to add 4 to the exception return address
        switch (c->mcause) {
        case EXCP_U_CALL:
        case EXCP_M_CALL:
            if (c->GPR1 == -1) { // YIELD
                ev.event = EVENT_YIELD; c->mepc += 4;
            } else if (c->GPR1 >= 0 && c->GPR1 <= 19) { 
                // NR_SYSCALL:20
                ev.event = EVENT_SYSCALL; c->mepc += 4;
            } else {
                assert("unknown exception event\n");
            }
        break;
        case 0x80000007: 
            // S-mode is not supported yet TODO
            ev.event = EVENT_IRQ_TIMER;
        break;
        default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  __am_switch(c);
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
    void *stack_end = kstack.end;
    Context *base = (Context *) ((uint8_t *)stack_end - sizeof(Context));
    // just pass the difftest
    //base->mstatus = 0x1800;
    const CsrMstatus_t mstatus_tmp = {
        .mpie = 1,
        .mie = 0,
        .mpp = PRIV_MODE_M,
    };

    // notice the MPIE will be restored to the MIE in nemu
    //base->mstatus |= (1 << 7); MPIE = 1;
    base->mstatus = mstatus_tmp.packed;
    base->pdir = NULL;
    base->np = PRIV_MODE_M;
    base->mepc = (uintptr_t)entry;
    base->gpr[2] = (uintptr_t)kstack.end; // sp
    base->gpr[10] = (uintptr_t)arg; // a0
    return base;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
    CsrMstatus_t mstatus_tmp;
    asm volatile("csrr %0, satp" : "=r"(mstatus_tmp.packed));
    return mstatus_tmp.mie;
}

void iset(bool enable) {
    CsrMstatus_t mstatus_tmp;
    asm volatile("csrr %0, satp" : "=r"(mstatus_tmp));
    mstatus_tmp.mie = enable;
    asm volatile("csrw mstatus, %0" : : "r"(mstatus_tmp));
}
