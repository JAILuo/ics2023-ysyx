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
#define IS_SYS_FILE_CALL(type) ((type) == SYS_read || \
                                (type) == SYS_write || \
                                (type) == SYS_close || \
                                (type) == SYS_lseek)
#define IS_SYS_OPEN(type) ((type) == SYS_open)

char *fd_name(int fd);
static inline void Strace(Context *c, intptr_t type, intptr_t a0, intptr_t a1, intptr_t a2) {
    #define FORMAT_STRACE "syscall: %s num: %d\n" \
                          "        reg: a0: %d  a1: %d  a2: %d\n" \
                          "        ret: %d\n"
    const char *file_name = (IS_SYS_OPEN(type)) ? ((const char *)a0) :
                (IS_SYS_FILE_CALL(type) ? fd_name((int)a0) : "no file"); 

    Log("\n[strace]" FORMAT_STRACE
        "        sfs open file: %s\n",
        syscall_name[type], type, a0, a1, a2, c->GPRx, file_name);
    #undef FORMAT_STRACE
}
#define strace() Strace(c, a[0], a[1], a[2], a[3])
#else
#define strace() 
#endif

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

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

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; // a7
  a[1] = c->GPR2; // a0
  a[2] = c->GPR3; // a1
  a[3] = c->GPR4; // a2

  switch (a[0]) {
    case SYS_exit: strace(); sys_exit(a[0]); break;
    case SYS_yield: c->GPRx = sys_yield(); break;
    case SYS_open: c->GPRx = fs_open((const char *)a[1], a[2], a[3]); break;
    case SYS_read: c->GPRx = fs_read(a[1], (void *)a[2],a[3]); break;
    case SYS_write: c->GPRx = fs_write(a[1], (const void *)a[2], a[3]); break;
    case SYS_close: c->GPRx = fs_close(a[1]); break;
    case SYS_lseek: c->GPRx = fs_lseek(a[1], a[2], a[3]); break;
    case SYS_brk: c->GPRx = 0; break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  strace();
}
