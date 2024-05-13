/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>

#include <isa.h>
#include <debug.h>
#include <cpu/cpu.h>
#include "sdb.h"
#include "utils.h"
#include <memory/vaddr.h>
/* 为什么写成这个就可以？ */
/* 默认搜索 src 目录下的文件/目录？ */

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
    if (!nemu_state.halt_ret) {
        nemu_state.state = NEMU_QUIT;
    }
  return -1;
}

static int cmd_si(char *args) {
    int step = 1;
    if (args) {
        sscanf(args, "%d", &step);
    }
    cpu_exec((uint64_t)step);
    return 0;
}

static int cmd_info(char *args) {
    if (args == NULL) {
        printf("Usage: info SUBCMD (info r / info w)\n");
        return 0;
    }

    if (args[0] == 'r') {
        isa_reg_display();
    } else if (args[0] == 'w') {
        display_wp();
    } else {
        printf("Please use info r or info w\n");
    }
    return 0;
}

bool is_hex(const char *str) {
    // 跳过字符串前面的空格字符
    str += strspn(str, " ");

    // 检查字符串是否为空
    if (*str == 0) {
        return false;
    }

    // 检查字符串是否以 "0x" 或 "0X" 开头
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        return true;; // 跳过 "0x" 前缀
    } else {
        return false;
    }

    /* 先不做这么多判断，就看开头是不是0x的 */
}

void display_mem2val(unsigned long addr, int num) {
    word_t value= vaddr_read((vaddr_t)addr, 4);
    for (int i = 0; i < num; i++) {
       value= vaddr_read((vaddr_t)addr + i * 4, 4);
       printf("0x%08lx: 0x%08x\n", addr + i * 4, value);
    }

}

/**
 * The default is to output 4 continuous bytes
 * n defaults to 1
 */
static int cmd_x(char *args) {
    if (!args) {
        printf("Usage: x N EXPR\n");
        return 0;
    }

    unsigned long addr = 0;

    char *str = strdup(args);
    Assert(str != NULL, "memory allocate error."); 

    char *token = strtok(str, " ");
    char *endptr;
    int num = strtol(token, &endptr, 10);
    if (*endptr == '\0') {
        // conversion success, token is decimal.
        token = strtok(NULL, " ");
    } else {
        // failed, token maybe a hex or an expression
        num = 1;
    }

    if (is_hex(token)) {
        // pass hex
        addr = strtoul(token, NULL, 16);
    } else {
        // pass expression
        bool success = true;
        addr = expr(token, &success);
        if (!success) {
            printf("eval failed.\n");
            free(str);
            str = NULL;
            return 0;
        }
    }

    display_mem2val(addr, num);

    free(str);
    str = NULL;

    return 0;
}

static int cmd_p(char *args) {
    if (!args) {
        printf("Usage: p expr\n");
        return 0;
    }
    bool success = true;
    int result = expr(args, &success);
    if (!success) {
        printf("eval failed.\n");
        return 0;
    }
    printf("uint32_t dec: %"PRIu32"\n", result);
    printf("int dec: %d\n", (int)result);
    printf("hex: 0x%x\n", (int)result);
    return 0;
}

static int cmd_w(char *args) {
    if (!args) {
        printf("Usage: w expr\n");
        return 0;
    }
    bool success = true;
    int result = expr(args, &success);
    if (success == false) {
        printf("error eval.\n");
        return 0;
    }
    watch_wp(args, result);

    return 0;
}

static int cmd_d(char *args) {
    char *wp_num = strtok(args, " ");
    if (!wp_num) {
        printf("Usage: d N\n");
    }
    int no = strtol(wp_num, NULL, 10);
    delete_wp(no);
    return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execute the next N commands and pause, N is 1 by default", cmd_si},
  { "info", "Print the states of register and watchpoint", cmd_info},
  { "x", "Evaluate the expression and print N consecutive 4 bytes in hexadecimal form ", cmd_x},
  { "p", "Display the value of an expression", cmd_p},
  { "w", "Pause the program when then expression expr changes", cmd_w},
  { "d", "Delete the the N watchpoint", cmd_d},

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
