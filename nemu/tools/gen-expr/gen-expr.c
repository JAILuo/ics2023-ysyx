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

// this should be enough 64KB
static char buf[65536] = {};
static int buf_index = 0;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

/**
 * 生成小于n的数字
 */
uint32_t choose(uint32_t n) {
    return rand() % n ;
}

void gen_space() {
    // 仅在操作符后插入空格
    const char *ops = " +-*/";
    if (strchr(ops, buf[buf_index - 1]) && buf_index > 0) {
        buf[buf_index++] = ' ';
    }
}

void gen_num() {
    uint32_t num = 0;
        do {
        num = choose(100);
    } while (num == 0); // 确保不生成数字零，以避免除以零

    // 将数字转换为 字符串并追加到 buf
    char num_str[4]; // 3位数 + 1个结束符 '\0'
    gen_space();
    
    snprintf(num_str, sizeof(num_str), "%u", num);

    strcpy(&buf[buf_index], num_str); // 复制数字字符串到 buf
    buf_index += strlen(num_str); // 更新 buf 的索引
}

void gen(char c) {
    buf[buf_index++] = c;
}

void gen_rand_op(bool allowTrailingOperator) {
    if (allowTrailingOperator && buf_index > 0 && buf[buf_index - 1] != ' ') {
        char op[4] = {'+', '-', '*', '/'};
        buf[buf_index++] = op[choose(4)];
    }
}

void gen_rand_expr(bool allowTrailingOperator) {
    // 溢出处理
    if (buf_index > sizeof(buf) - 16) {
        return;
    }

    bool hasExpr = false;

    switch (choose(3)) {
        case 0: 
            gen_num();
            hasExpr = true;
            break;
        case 1:
            if (choose(2) == 0) { // 50% 的概率生成嵌套括号
                gen('(');
                gen_rand_expr(false); // 嵌套括号内不允许尾随运算符
                gen(')');
                hasExpr = true;
            }
            break;
        default: 
            if (!hasExpr) {
                gen_num();
                hasExpr = true;
            }
            gen_rand_op(false); // 表达式间的运算符不允许尾随运算符
            break;
    }

    if (hasExpr && allowTrailingOperator) {
        gen_rand_op(true); // 表达式后可以跟一个尾随运算符
    }
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
    gen_rand_expr(false);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
