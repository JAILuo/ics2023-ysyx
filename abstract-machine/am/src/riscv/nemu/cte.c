#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <stdint.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
    if (user_handler) {
    //printf("in __am_irq_handle,c:%p c->sp:%p\n", c, c->gpr[2]);
        Event ev = {0};
        switch (c->mcause) {
          	case 0xb:
            	if (c->GPR1 == -1) { // YIELD
                    //printf("yield\n");
                    ev.event = EVENT_YIELD; c->mepc += 4;
                } else if (c->GPR1 >= 0 && c->GPR1 <= 19){ 
                    // system call (include sys_yield)
                    // NR_SYSCALL:20
                    //printf("syscall:%d\n", c->GPR1);
                	ev.event = EVENT_SYSCALL; c->mepc += 4;   
            	} else {
                	printf("unknown exception event\n");
            	}
            break;
        default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

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
    base->mstatus = 0x1800;
    base->pdir = NULL;
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
  // pass the difftest
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
