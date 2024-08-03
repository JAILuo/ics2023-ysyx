#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;

void call_main(uintptr_t *args) {
    // char * for portability
    char *base = (char *)args;

    int argc = *((int *)base);
    printf("[call_main] argc: %d addr: %p\n", argc, &argc);
    base += sizeof(int);

    char **argv = (char **)base;
    printf("[call_main] argv addr:%p\n", argv);
    printf("[call_main] argv[0]:%s\n", argv[0]);
    base += sizeof(char *) * (argc + 1);

    char **envp = (char **)base;
    environ = envp;

    exit(main(argc, argv, envp));
    assert(0);
}
