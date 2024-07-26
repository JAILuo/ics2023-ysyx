#include <nterm.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <SDL.h>

#define test

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

#ifndef test
#define buf_size 128
static char fname[buf_size];
static char arg_first[buf_size];
static void sh_handle_cmd(const char *cmd) {
    //char tmp_cmd[64];
    //strcpy(tmp_cmd, cmd);
    //tmp_cmd[strlen(cmd) - 1] = '\0';
    //setenv("PATH", "/bin", 0);
    //execvp(tmp_cmd, NULL);
    if (cmd == NULL) return;
    printf("cmd: %s\n", cmd);
    if (strncmp(cmd, "echo", 4) == 0) {
        if (strlen(cmd) == 5) sh_printf("\n");
        else sh_printf("%s", cmd + 5);
    } else {
        if (strlen(cmd) > buf_size) {
            sh_printf("command too long\n");
            return;
        }

        int arg_first_offset = 0;
        while (cmd[arg_first_offset] != ' ') {
            arg_first_offset++;
            if (cmd[arg_first_offset] == '\n') {
                arg_first_offset = -1;
                break;
            }
        }
        memset(fname, '\0', buf_size);
        memset(arg_first, '\0', buf_size);

        if (arg_first_offset > 0) {
            strncpy(fname, cmd, arg_first_offset);
            printf("in nterm, fname:%s\n",fname);
            strncpy(arg_first, cmd + arg_first_offset + 1, strlen(cmd) - arg_first_offset);
            printf("in nterm, argv:%s\n", arg_first);
            char *argv[] = {fname, arg_first, NULL};
            execve(fname, argv, NULL);
        } else {
            strncpy(fname, cmd, strlen(cmd) - 1);
            char *argv[] = {fname, NULL};
            execve(fname, argv, NULL);
        }
        // execvp(fname, argv);
    }
}
#endif

#ifdef test
#define argc_max 128
static void sh_handle_cmd(const char *cmd) {
    if (cmd == NULL) return;
    if (strncmp(cmd, "echo", 4) == 0) { // built-in command
        if (strlen(cmd) == 5) sh_printf("\n");
        else sh_printf("%s", cmd + 5);
    } else {
        int argc = 0;

        char *cmd_without_newline = (char *)malloc(sizeof(char) * strlen(cmd));
        memset(cmd_without_newline, 0, strlen(cmd));
        strncpy(cmd_without_newline, cmd, strlen(cmd) - 1);

        char **argv = (char **)malloc(sizeof(char *) * argc_max);

        char *cur = strtok(cmd_without_newline, " ");
        assert(cur != NULL); // avoid cur is ""

        char *fname = (char *)malloc(sizeof(char) * (strlen(cur) + 1));
        memset(fname, 0, strlen(cur) + 1);
        strcpy(fname, cur);
        printf("[sh_handle_cmd] filename: %s at %p\n", fname, fname);

        while (cur) {
            printf("[sh_handle_cmd] argv[%d]: %s at %p\n", argc, cur, cur);
            argv[argc] = cur;
            cur = strtok(NULL, " ");
            argc++;
            if (argc == argc_max) {
                sh_printf("too many arguments\n");
                free(argv);
                free(fname);
                free(cmd_without_newline);
                return;
            }
        }
        argv[argc] = NULL;

        execve(fname, argv, NULL);

        fprintf(stderr, "\033[31m[ERROR]\033[0m Exec %s failed.\n\n", fname);
        free(argv);
        free(fname);
        free(cmd_without_newline);
    }
}
#endif

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
