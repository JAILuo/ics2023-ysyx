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

static char *argv_pal[] = {"/bin/pal", "--skip", NULL};
//static char *argv_pal[] = {"/bin/pal", NULL};
static char *envp[] = {NULL};
static char *argv[] = {NULL};
//static char *argv_exec_test[] = {"/bin/exec-test",  NULL};
//static char *argv_menu[] = {"/bin/menu",  NULL};
//static char *argv_nterm[] = {"/bin/nterm",  NULL};
void init_proc() {
  Log("Initializing processes...");

  //Log("pcb_boot:%p\n", pcb_boot);
  //context_uload(&pcb[0], "/bin/menu", argv_menu, envp);
  //context_uload(&pcb[1], "/bin/hello", argv, envp);
  //context_uload(&pcb[0], "/bin/nterm", argv_nterm, envp);
  //context_uload(&pcb[0], "/bin/exec-test", argv_exec_test, envp);
  //context_uload(&pcb[0], "/bin/pal", argv_pal, envp);
  //context_kload(&pcb[1], hello_fun, "A");
  //Log("pcb[0]: %p, pcb[0]->cp:%p, pcb0->cp->sp:%p", pcb[0], pcb[0].cp, pcb[0].cp->gpr[2]);
  //Log("pcb[1]: %p, pcb[1]->cp:%p, pcb1->cp->sp:%p", pcb[1], pcb[1].cp, pcb[1].cp->gpr[2]);
  //printf("in init_proc pcb0->cp->mepc:%p\n",pcb[0].cp->mepc);
  //naive_uload(&pcb[0], "/bin/dummy");

  //context_kload(&pcb[0], hello_fun, "A");
  context_uload(&pcb[0], "/bin/hello", argv, envp);
  context_uload(&pcb[1], "/bin/pal", argv_pal, envp);
  context_uload(&pcb[2], "/bin/bird", argv, envp);
  context_uload(&pcb[3], "/bin/nslider", argv, envp);
  switch_boot_pcb();

}

int fg_pcb = 1;
static int schedule_count = 0;
Context* schedule(Context *prev) {
    current->cp = prev;
    // 增加调度计数
    schedule_count++;

    // 检查是否应该切换到 hello_fun 线程
    if (schedule_count % 20 == 0) { // 每10次调度，切换到 hello_fun 一次
        current = &pcb[0];
        //current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
    } else {
        // 否则，保持当前进程不变，优先执行 menu 进程
        current = &pcb[fg_pcb];
    }

    return current->cp;
}

/*
Context* schedule(Context *prev) {
    //current = &pcb[0]; // for pa4.2 test

    current->cp = prev;
    //Log("prev->cp:%p", current->cp);
    current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
    //Log("new->cp:%p", current->cp);

    return current->cp;
}
*/
