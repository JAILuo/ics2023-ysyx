#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;

void call_main(uintptr_t *args) {
    uintptr_t *base = args;
    printf("in crt. argc:%d\n", *((int *)base));
    int argc = *((int *)base);
    base++;

    char **argv = (char **)base;
    printf("in crt. argv[0]:%s\n", argv[0]);
    if (argc >= 2 ) {
        printf("in crt. argv[1]:%s\n", argv[1]);
    }
    base += (argc + 1);

    char **envp = (char **)base;
    environ = envp;
    exit(main(argc, argv, envp));
    assert(0);
}
