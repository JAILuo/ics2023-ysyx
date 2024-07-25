#include <nterm.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <SDL.h>

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

#define buf_size 128
static char fname[buf_size];
static char arg_first[buf_size];
static void sh_handle_cmd(const char *cmd) {
    /*
    char tmp_cmd[64];
    strcpy(tmp_cmd, cmd);
    tmp_cmd[strlen(cmd) - 1] = '\0';
    setenv("PATH", "/bin", 0);
    execvp(tmp_cmd, NULL);
    */
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
        memset(fname, 0, buf_size);
        memset(arg_first, 0, buf_size);

        if (arg_first_offset > 0) {
            strncpy(fname, cmd, arg_first_offset);
            strncpy(arg_first, cmd + arg_first_offset + 1, strlen(cmd) - arg_first_offset);
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
