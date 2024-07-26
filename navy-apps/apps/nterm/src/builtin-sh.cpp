#include <nterm.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
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


#ifdef test
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

// https://github.com/torvalds/linux/blob/36ee98b555c00c5b360d9cd63dce490f4dac2290/lib/argv_split.c
static int count_argc(const char *str) {
  int count = 0;
  bool was_space;

  for (was_space = true; *str; str++) {
    if (isspace(*str)) {
      was_space = true;
    } else if (was_space) {
      was_space = false;
      count++;
    }
  }

  return count;
}
/**
 * argv_free - free an argv
 * @argv: the argument vector to be freed
 *
 * Frees an argv and the strings it points to.
 */
void argv_free(char **argv) {
  argv--;
  free(argv[0]);
  free(argv);
}


/**
 * argv_split - split a string at whitespace, returning an argv
 * @gfp: the GFP mask used to allocate memory
 * @str: the string to be split
 * @argcp: returned argument count
 *
 * Returns: an array of pointers to strings which are split out from
 * @str.  This is performed by strictly splitting on white-space; no
 * quote processing is performed.  Multiple whitespace characters are
 * considered to be a single argument separator.  The returned array
 * is always NULL-terminated.  Returns NULL on memory allocation
 * failure.
 *
 * The source string at `str' may be undergoing concurrent alteration via
 * userspace sysctl activity (at least).  The argv_split() implementation
 * attempts to handle this gracefully by taking a local copy to work on.
 */
char **argv_split(const char *str, int *argcp) {
  char *argv_str;
  bool was_space;
  char **argv, **argv_ret;
  int argc;

  argv_str = (char *)malloc(strlen(str) + 1);
  if (!argv_str)
    return NULL;
  strcpy(argv_str, str);

  argc = count_argc(argv_str);
  argv = (char **)calloc(argc + 2, sizeof(*argv));
  if (!argv) {
    free(argv_str);
    return NULL;
  }

  *argv = argv_str;
  argv_ret = ++argv;
  for (was_space = true; *argv_str; argv_str++) {
    if (isspace(*argv_str)) {
      was_space = true;
      *argv_str = 0;
    } else if (was_space) {
      was_space = false;
      *argv++ = argv_str;
    }
  }
  *argv = NULL;

  if (argcp)
    *argcp = argc;
  return argv_ret;
}

static void sh_handle_cmd(const char *cmd) {
  int argc;
  char **argv = argv_split(cmd, &argc);

  if (argv == NULL) {
    sh_printf("Cannot split argv\n");
    return;
  }
  if (argc == 0) {
    argv_free(argv);
    return;
  }

  if (!strcmp(argv[0], "exit")) {
    exit(0);
  }

  setenv("PATH", "/usr/bin:/bin", false);
  const int ret = execvp(argv[0], argv);
  if (ret == -1) {
    sh_printf("%s: errno %d: %s\n", argv[0], errno, strerror(errno));
    argv_free(argv);
    return;
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
