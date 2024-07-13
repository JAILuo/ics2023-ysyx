#include <common.h>
#include "syscall.h"

const char *syscall_name[] = {
    [SYS_exit] = "exit",
    [SYS_yield] = "yield",
    [SYS_open] = "open",
    [SYS_read] = "read",
    [SYS_write] = "write",
    [SYS_kill] = "kill",
    [SYS_getpid] = "getpid",
    [SYS_close] = "close",
    [SYS_lseek] = "lseek",
    [SYS_brk] = "brk",
    [SYS_fstat] = "fstat",
    [SYS_time] = "time",
    [SYS_signal] = "signal",
    [SYS_execve] = "execve",
    [SYS_fork] = "fork",
    [SYS_link] = "link",
    [SYS_unlink] = "unlink",
    [SYS_wait] = "wait",
    [SYS_times] = "times",
    [SYS_gettimeofday] = "gettimeofday"
    // more...
};

//#define CONFIG_STRACE 
#ifdef CONFIG_STRACE
#define strace() \
    Log("\n[strace]" "syscall: %s num: %d\n" \
        "        reg: a0: %d  a1: %d  a2: %d\n" \
        "        ret: %d\n", \
           syscall_name[c->GPR1], c->GPR1, c->GPR2, c->GPR3, c->GPR3, c->GPRx); 
#else
#define strace() 
#endif

void yield();
void halt(int code);
void putch(char ch);

int sys_yield() {
    yield();
    return 0;
}

void sys_exit(int code) {
    halt(code);
}
int sys_write(int fd, const void *buf, int count) {
    if (fd == 1 || fd == 2) {
        for (int i = 0; i < count; i++)
            putch(*((const char *)buf+ i));
    }
    return count;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; // a7
  a[1] = c->GPR2; // a0
  a[2] = c->GPR3; // a1
  a[3] = c->GPR4; // a2

  switch (a[0]) {
    case SYS_exit: strace(); sys_exit(a[0]); break;
    case SYS_yield: c->GPRx = sys_yield(); break;
    case SYS_write: c->GPRx = sys_write(a[1], (char *)a[2], a[3]); break;
    case SYS_brk: c->GPRx = 0; break;
    default: /*panic("Unhandled syscall ID = %d", a[0]); */
  }
  strace();
}
