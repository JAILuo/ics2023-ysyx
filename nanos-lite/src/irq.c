#include <common.h>
#include <stdbool.h>

extern void do_syscall(Context *c);
extern Context* schedule(Context *prev);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: 
        //printf("before schedule, c->mepc:%p\n", c->mepc);
        //printf("before, a0(filename):%p, a1(argv):%p, a2(envp): %p\n", 
        //      c->GPR2, c->GPR3, c->GPR4);
        //Log("in user_handle, before schedule, Context: %p, c->sp:%p", c, c->gpr[2]);
        c = schedule(c); 
        //printf("test dummy...\n");
        //Log("in user_handle, after schedule, Context: %p, c->sp:%p", c, c->gpr[2]);
        //printf("after schedule, c->mepc:%p\n", c->mepc);
        //printf("after, a0(filename):%p, a1(argv):%p, a2(envp): %p\n", 
        //      c->GPR2, c->GPR3, c->GPR4);
        break;
    case EVENT_SYSCALL: do_syscall(c); break;
    //case EVENT_PAGEFAULT: break;
    case EVENT_IRQ_TIMER:
        //Log("IRQ before: %p", c);
        //printf("before schedule, c->mepc:%p\n", c->mepc);
        c = schedule(c);
        //Log("IRQ after: %p", c);
        //printf("after schedule, c->mepc:%p\n", c->mepc);
        break;
    case EVENT_IRQ_IODEV: break;
    case EVENT_IRQ_SOFTWARE: break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
    Log("Initializing interrupt/exception handler...");
    cte_init(do_event);
}
