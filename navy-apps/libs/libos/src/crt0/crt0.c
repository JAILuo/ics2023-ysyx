#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;

void call_main(uintptr_t *args) {
    // char * for portability
    char *base = (char *)args;

    int argc = *((int *)base);
    base += sizeof(int);

    char **argv = (char **)base;
    base += sizeof(char *) * (argc + 1);

    char **envp = (char **)base;
    environ = envp;

    exit(main(argc, argv, envp));
    assert(0);
}
