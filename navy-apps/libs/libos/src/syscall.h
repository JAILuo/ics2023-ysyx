#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// use X macro like AM KEY(abstract-machine/am/include/amdev.h)
#define SYSCALLS(_) \
    _(exit),       \
    _(yield),      \
    _(open),       \
    _(read),       \
    _(write),      \
    _(kill),       \
    _(getpid),     \
    _(close),      \
    _(lseek),      \
    _(brk),        \
    _(fstat),      \
    _(time),       \
    _(signal),     \
    _(execve),     \
    _(fork),       \
    _(link),       \
    _(unlink),     \
    _(wait),       \
    _(times),      \
    _(gettimeofday) \
    
#define SYSCALL_NAMES(name) SYS_##name
#define SYSCALL_STRINGS_NAMES(name) [SYS_##name] = #name

enum {
    SYSCALLS(SYSCALL_NAMES)
};

#endif

