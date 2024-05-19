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

#include <readline/readline.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

static char buf[128] = {};
static int buf_index = 0;
static char code_buf[128 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

#define MAX_EXPR_LENGTH 128// 确保有一个字符留给空字符 '\0'
/**
 * 生成小于n的数字
 */
uint32_t choose(uint32_t n) {
    return rand() % n ;
}

void gen_space(void) {
    if (choose(2)) {
        buf[buf_index++] = ' ';
    }
}

void gen_num() {
    uint32_t num;
    do {
        num = choose(100);
    } while (num == 0);

    char num_str[4];
    snprintf(num_str, sizeof(num_str), "%u", num);

    if (buf_index + strlen(num_str) < MAX_EXPR_LENGTH) {
        strcpy(&buf[buf_index], num_str);
        buf_index += strlen(num_str);
    }
}

void gen(char c) {
    buf[buf_index++] = c;
}

void gen_rand_op() {
    char op[4] = {'+', '-', '*', '/'};
    int op_index = choose(4);
    if (buf_index < MAX_EXPR_LENGTH - 1) {
        buf[buf_index++] = op[op_index];
    }
}

bool is_last_operator(void) {
    char last_char = buf[buf_index - 1];
    return strchr("+-*/ ", last_char) != NULL;
}

/* if (, return true  */
bool is_last_leftparens(void) {
    if (buf_index == 0) return false;
    char last_char = buf[buf_index - 1];
    return strchr(")", last_char) != NULL;
}

static void gen_rand_expr() {
    switch (choose(3)) {
        case 0:
            if (buf_index < MAX_EXPR_LENGTH) {
                if (!is_last_leftparens()) {
                    gen_num();
                } else {
                    gen_rand_expr();
                }
            }
            break;
        case 1:
            if (buf_index < MAX_EXPR_LENGTH) {
                if (buf[0] != '\0' && is_last_operator()) {
                    gen('(');
                    gen_rand_expr();
                    gen(')');
                } else {
                    gen_rand_expr();
                }
            }
            break;
        default:
            if (buf_index < MAX_EXPR_LENGTH) {
                gen_rand_expr();
                if (!is_last_operator()) {
                    gen_rand_op();
                }
                gen_rand_expr();
            }
            break;
    }
}
// 确保 buf 以空字符结尾
void ensure_buf_terminated(void) {
    if (buf_index > 0 && buf_index < sizeof(buf)) {
        buf[buf_index] = '\0';
    }
}

// 使用 snprintf 替代 sprintf
void generate_code_expression(void) {
    ensure_buf_terminated(); // 确保 buf 以空字符结尾
    // 检查 buf 是否超出了 code_buf 的容量
    if (strlen(buf) >= sizeof(code_buf) - strlen(code_format) - 1) {
        fprintf(stderr, "Error: Expression too long.\n");
        return;
    }
    // 使用 snprintf 避免缓冲区溢出
    snprintf(code_buf, sizeof(code_buf), code_format, buf);
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    generate_code_expression(); // 生成代码表达式并检查溢出
   
    if (strlen(code_buf) < sizeof(code_buf)) {
        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        // filter div zero by compiler
        //int ret = system("gcc /tmp/.code.c -O2 -Wall -Werror -o /tmp/.expr");
        int ret = system("gcc /tmp/.code.c -O2 -Wall -Wno-error=overflow -o /tmp/.expr");
        if (ret != 0) continue;

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        int result;
        ret = fscanf(fp, "%d", &result);
        pclose(fp);

        printf("%u %s\n", result, buf);
        
    } else {
    // 如果表达式太长，跳过当前迭代
        continue;
    }
    /*
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // filter div zero by compiler
    //int ret = system("gcc /tmp/.code.c -O2 -Wall -Werror -o /tmp/.expr");
    int ret = system("gcc /tmp/.code.c -O2 -Wall -Wno-error=overflow -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
    */
  }
  return 0;
}
