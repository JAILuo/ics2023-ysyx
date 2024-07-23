#include <stdint.h>
#include <proc.h>

#define MAX_NR_PROC 4

uintptr_t naive_uload(PCB *pcb, const char *filename);

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (const char *)arg, j);
    j ++;
    yield();
  }
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
    Area stack = {
        .start = pcb->stack,
        .end = pcb->stack + STACK_SIZE
    };
    pcb->cp = kcontext(stack, entry, arg);
}

void context_uload(PCB *pcb, const char *process_name) {
    uintptr_t entry = naive_uload(pcb, process_name);
    Area stack = {
        .start = heap.end - STACK_SIZE,
        .end   = heap.end
    };
    Log("name: %s", process_name);
    Log("entry: %d", entry);
    Log("stack.start: %d, stack.end: %d", stack.start, stack.end);

    pcb->cp = ucontext(&pcb->as, stack, (void *)entry);
    pcb->cp->GPRx = (uintptr_t)heap.end;
}

void init_proc() {
  Log("Initializing processes...");

  context_kload(&pcb[0], hello_fun, "123");
  //context_uload(&pcb[0], "/bin/hello");
  context_uload(&pcb[1], "/bin/pal");
  switch_boot_pcb();

  // load program here
  //naive_uload(NULL, "/bin/pal");
  //naive_uload(NULL, "/bin/bmp-test");
}

Context* schedule(Context *prev) {
    current->cp = prev;
    current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
    return current->cp;
}

