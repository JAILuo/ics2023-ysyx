#include <common.h>

extern void do_syscall(Context *c);
extern Context* schedule(Context *prev);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: 
        //printf("before schedule, c->mepc:%p\n", c->mepc);
        //printf("before, a0(filename):%p, a1(argv):%p, a2(envp): %p\n", 
        //      c->GPR2, c->GPR3, c->GPR4);
        c = schedule(c); 
        //printf("after schedule, c->mepc:%p\n", c->mepc);
        //printf("after, a0(filename):%p, a1(argv):%p, a2(envp): %p\n", 
        //      c->GPR2, c->GPR3, c->GPR4);
        break;
    case EVENT_SYSCALL: do_syscall(c); break;
    case EVENT_IRQ_TIMER: printf("event IRQ\n"); break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
