#include <stdint.h>
#include <proc.h>
#include <loader.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    if (j++ > 100) {
        j = 0;
        Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (const char *)arg, j);
    }
    j ++;
    yield();
  }
}

void init_proc() {
  Log("Initializing processes...");
  //char *argv_pal[] = {"/bin/pal", "--skip", NULL};
  //char *argv_pal[] = {"/bin/pal", NULL};
  char *envp[] = {NULL};
  char *argv_exec_test[] = {"/bin/exec-test",  NULL};
  //char *argv_menu[] = {"/bin/menu",  NULL};
  //char *argv_nterm[] = {"/bin/nterm",  NULL};
  
  context_uload(&pcb[0], "/bin/exec-test", argv_exec_test, envp);
  //context_uload(&pcb[0], "/bin/nterm", argv_nterm, envp);
  //context_uload(&pcb[0], "/bin/pal", argv_pal, envp);
  //context_uload(&pcb[0], "/bin/menu", argv_menu, envp);
  context_kload(&pcb[1], hello_fun, "A");
  //context_uload(&pcb[0], "/bin/pal", argv_pal, envp);
  //printf("in init_proc pcb0->cp->mepc:%p\n",pcb[0].cp->mepc);
  switch_boot_pcb();
}

Context* schedule(Context *prev) {
    current->cp = prev;
    //printf("prev->cp:%p\n", current->cp);
    current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
    //printf("new->cp:%p\n", current->cp);
    return current->cp;
}

