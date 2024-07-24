#include <stdint.h>
#include <proc.h>

#define MAX_NR_PROC 4

uintptr_t naive_uload(PCB *pcb, const char *filename);
void context_kload(PCB *pcb, void (*entry)(void *), void *arg);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

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

void init_proc() {
  Log("Initializing processes...");
  //char *argv_pal[] = {"/bin/pal", "--skip", NULL};
  char *argv_exec_test[] = {"/bin/exec-test", NULL};
  context_uload(&pcb[0], "/bin/exec-test", argv_exec_test, NULL);
  //context_kload(&pcb[1], hello_fun, "A");
  //context_uload(&pcb[0], "/bin/hello");
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

