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

void gen_space(void) {
    if (choose(2)) {
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
    
    snprintf(num_str, sizeof(num_str), "%u", num);

    strcpy(&buf[buf_index], num_str); // 复制数字字符串到 buf
    buf_index += strlen(num_str); // 更新 buf 的索引
}

void gen(char c) {
    buf[buf_index++] = c;
}

void gen_rand_op() {
    char op[4] = {'+', '-', '*', '/'};
    int op_index = choose(4); 
    buf[buf_index++] = op[op_index];
}

bool is_last_operator(void) {
    char last_char = buf[buf_index - 1];
    return strchr("+-*/ ", last_char) != NULL;
}

// 前面是() 那就返回true
bool is_last_leftparens(void) {
    if (buf_index == 0) return false;
    char last_char = buf[buf_index - 1];
    return strchr(") ", last_char) != NULL;
}
bool is_last_rightparens(void) {
    if (buf_index == 0) return false;
    char last_char = buf[buf_index - 1];
    return strchr("( ", last_char) != NULL;
}

// 是不是需要将左右括号分开？ 是的
bool is_last_space(void) {
    if (buf_index == 0) return false;
    char last_char = buf[buf_index - 1];
    return strchr(" ", last_char) != NULL;
}

// 检查最后一个字符是否是数字
bool is_last_num(void) {
    if (buf_index == 0) return false; // 如果缓冲区为空，则没有最后一个字符
    char last_char = buf[buf_index - 1];
    return isdigit((unsigned char)last_char) != 0; // 使用isdigit检查是否为数字
}

/*
void gen_rand_expr() {
  switch (choose(3)) {
    case 0:
        //if (buf_index > 0 && 
         //   gen_rand_op();
        //} else {
        //}
             //(is_last_leftparens() != true && is_last_rightparens() != true )) {
        if (buf_index > 0 && is_last_operator() != true 
            && is_last_leftparens() != true) {
            gen_rand_op();
        }  
        gen_num();

        break;
    case 1:
        gen('(');
        gen_rand_expr();
        gen(')');
        break;
    default:
        gen_rand_expr();
        if (is_last_leftparens() != true) {
            gen_rand_op();
        }
        gen_rand_expr();
        break;
  }
}
*/
static void gen_rand_expr() {
    switch (choose(3)) {
        case 0:
            if( buf[strlen(buf) - 1]!=')')
            {
            gen_num(); // 生成随机数字
            }
            else
            {
              gen_rand_expr();
            }
            break;
        case 1:
              // 避免在操作数之后立即插入左括号，而是在操作符之后插入左括号
            if (buf[0] != '\0' &&  strchr("+-*/", buf[strlen(buf) - 1]))
            {
                //strcat(buf, "("); // 将左括号添加到缓冲区末尾
                gen_rand_expr(); // 递归生成随机表达式
                //strcat(buf, ")"); // 将右括号添加到缓冲区末尾
            }
            else
            {
                gen_rand_expr(); // 递归生成随机表达式
            }
            break;
        default:
            gen_rand_expr(); // 递归生成随机表达式
            gen_rand_op(); // 生成随机操作符
            gen_rand_expr(); // 递归生成随机表达式
            break;
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
    gen_rand_expr();

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
