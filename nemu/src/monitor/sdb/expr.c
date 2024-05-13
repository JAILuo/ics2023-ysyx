/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <inttypes.h>
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <assert.h>
#include <limits.h>
#include <readline/chardefs.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "macro.h"
#include "memory/vaddr.h"
#include "stack.h"

enum {
  TK_NOTYPE = 256,

  TK_ADD = 0,
  TK_SUB = 1,
  TK_MUL = 2,
  TK_DIV = 3,
  TK_EQ = 4,
  TK_NEQ = 5,
  TK_AND = 6,
  TK_OR = 7,

  TK_RIGHT_PAR = 8,
  TK_LEFT_PAR = 9,

  TK_NEG = 10,

  TK_NUM = 11,
  TK_REG_NAME = 12,

  TK_DEREF = 13,

  // 更多运算符：<= >=
  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE}, // spaces

    {"\\(", TK_LEFT_PAR},
    {"\\)", TK_RIGHT_PAR},

    /* hex and dec */
    {"0[xX][0-9a-fA-F]+|[0-9]+", TK_NUM},
   
    // register name (确保在数字之后，这样数字不会被当作单词匹配)
    {"\\$\\w+|\\w+", TK_REG_NAME}, // 有不是$开头的

    /* comparison operators */
    {"!=", TK_NEQ},
    {"==", TK_EQ},

    /* logical operator */
    {"&&", TK_AND},
    {"\\|\\|", TK_OR},

    {"\\+", TK_ADD},    
    {"-", TK_SUB},      // sub and neg
    {"\\*", TK_MUL},    // mul and deref. 
    {"/", TK_DIV},

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

static int add_token(char *substr_start, int substr_len, int i);
void string_2_num(int p, int q);

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}
typedef struct token {
  int type;
  char str[128];
  int num_value;
} Token;

static Token tokens[128] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        i = add_token(substr_start, substr_len, i);
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int add_token(char *substr_start, int substr_len, int i) {
    switch (rules[i].token_type) {
        case TK_NOTYPE:
            break;

        case TK_ADD:
            tokens[nr_token++].type = TK_ADD;
            break;

        case TK_SUB: // sub and negative.
            tokens[nr_token++].type = TK_SUB;
            break;

        case TK_MUL: // mul and
            tokens[nr_token++].type = TK_MUL;
            break;

        case TK_DIV:
            tokens[nr_token++].type = TK_DIV;
            break;

        case TK_NUM: // 比如说传进来一个数字 1234, 不能只判断1, 后面就没了。
            tokens[nr_token].type = TK_NUM;
            // 怎么判断一直到数字？直接在这个 token 的 substr_len 即可.
            if (substr_len > sizeof(tokens[nr_token].str) - 1) {
                substr_len = sizeof(tokens[nr_token].str) - 1;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

        case TK_RIGHT_PAR:
            tokens[nr_token++].type = TK_RIGHT_PAR;
            break;

        case TK_LEFT_PAR:
            tokens[nr_token++].type = TK_LEFT_PAR;
            break;

        case TK_REG_NAME:
            tokens[nr_token].type = TK_REG_NAME;
            // 复制寄存器名称
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

        case TK_EQ:
            tokens[nr_token++].type = TK_EQ;
            break;

        case TK_NEQ:
            tokens[nr_token++].type = TK_NEQ;
            break;

        case TK_AND:
            tokens[nr_token++].type = TK_AND;
            break;

        case TK_OR:
            tokens[nr_token++].type = TK_OR;

        default:
            printf("No rules were mathced with %s\n", substr_start);
            break;
    }
    return i; 
}

bool check_parentheses(int p, int q) {
    if (tokens[p].type != TK_LEFT_PAR || tokens[q].type != TK_RIGHT_PAR) {
        return false;
    }

    Stack *stack = Stack_create();
    for (int i = p + 1; i < q; i++) {
        if (tokens[i].type == TK_LEFT_PAR) {
            Stack_push(stack, &tokens[i].type);
        } else if (tokens[i].type == TK_RIGHT_PAR) {
            if (Stack_count(stack) == 0) {
                Stack_destroy(stack);
                return false;
            }
            Stack_pop(stack); 
        }
    }

    // 栈非空，有未匹配的左括号
    if (Stack_count(stack) != 0) {
        Stack_destroy(stack);
        return false;
    }

    Stack_destroy(stack);
    return true;
}

/**
 * Operator precedence 
 * Refer to the C Language manual
 */
int get_priority(int op_type) {
    switch (op_type) {
        case TK_ADD:
        case TK_SUB:
            return 1;
        case TK_MUL:
        case TK_DIV:
            return 2;
        case TK_EQ:
        case TK_NEQ:
            return 3;
        case TK_AND:
        case TK_OR:
            return 4;
        default:return -1; // Default priority for non-operators
  }
}

/**
 * 寻找主运算符
 *
 * @note 非运算符的token不是主运算符.
 * @note 出现在一对括号中的token不是主运算符
 * @note 主运算符的优先级在表达式中是最低的?
 *       这里我一直有点难以理解，有点怪怪的？
 */
int find_main_operator(int p, int q, int *min_priority) {
    int op = -1;
    int in_parens = 0; // 标记运算符是否在括号内
    Stack *stack = Stack_create();
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LEFT_PAR) {
            Stack_push(stack, &tokens[i].type);
            in_parens++;
        } else if (tokens[i].type == TK_RIGHT_PAR) {
            if (Stack_count(stack) > 0) {
                Stack_pop(stack);
                in_parens--;
            } else {
                // 如果栈为空，说明右括号没有匹配的左括号
                printf("Error: Unmatched right parenthesis\n");
                Stack_destroy(stack);
                return -1;
            }
        } else if (tokens[i].type == TK_NOTYPE) {
            continue;
        } else if (tokens[i].type >= TK_ADD && tokens[i].type <= TK_OR) {
            // 只有在不在括号内时，才检查运算符
            if (in_parens == 0) {
                int priority = get_priority(tokens[i].type);
                if (priority <= *min_priority) { 
                    // 注意这里是 <= 
                    // 因为运算是从左到右
                    // 但是主运算符是优先级最低的。
                    *min_priority = priority;
                    op = i;
                }
            }
        }
    }
    // 检查栈是否为空，确保所有括号都已匹配
    if (Stack_count(stack) != 0) {
        printf("Error: Unmatched left parenthesis\n");
        Stack_destroy(stack);
        return -1;
    }
    // 销毁栈
    Stack_destroy(stack);
    return op;
}

int compute(int op_type, int val1, int val2) {
    switch (op_type) {
        case TK_ADD: return val1 + val2;
        case TK_SUB: return val1 - val2;
        case TK_MUL: return val1 * val2;
        case TK_DIV:
            if (val2 == 0) {
                fprintf(stderr, "Error: Division by zero.\n");
                exit(EXIT_FAILURE);
            }
            return val1 / val2;
        case TK_EQ: return val1 == val2;
        case TK_NEQ: return val1 != val2;
        case TK_AND: return val1 && val2;
        case TK_OR: return val1 || val2;
        default:
            fprintf(stderr, "Invalid operation type: %d\n", op_type);
            exit(EXIT_FAILURE);
    }
}

/* Single token.
 * For now this token should be a number.
 * Return the value of the number.
 */
int eval_operand(int p, bool *success) {
    switch (tokens[p].type) {
        case TK_NUM: // dec and hex
            return tokens[p].num_value;
            break;
        case TK_REG_NAME:
            return isa_reg_str2val(tokens[p].str, success);
        default:
            printf("error_eval, expect number but got %d\n", tokens[p].type);
            return INT_MAX;
            break;
    }
}

/*
 * 在token表达式(tokens)中指示一个子表达式,
 * 使用两个整数 p 和 q 来指示这个子表达式的开始位置和结束位置.
 */
int eval(int p, int q) {
    int op_type;
    if (p > q) {
        /* Bad expression */
        printf("error_eval, index error.\n");
        return INT_MAX;
    } else if (p == q) {
        bool success = true;
        return eval_operand(p, &success);
    } else if (check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses. nice!
         */
        return eval(p + 1, q - 1);
    } else {
        int op = -1;
        int min_priority = INT_MAX;

        op = find_main_operator(p, q, &min_priority);
        if (op == -1) {
            printf("No operator found.\n");
            return INT_MAX;
        }
        op_type = tokens[op].type;

        int val1 = eval(p, op - 1);
        int val2 = eval(op + 1, q);
        return compute(op_type, val1, val2);
    }
}

/**
 * Check and convert the negative mark. 
 */
void is_neg(void) {
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == TK_SUB) {
      if ((i == 0) ||
          (tokens[i - 1].type != TK_NUM && tokens[i + 1].type == TK_NUM)) {
        tokens[i].type = TK_NEG;
      }
    }
  }
}

/**
 * 从start往后的内容，都往前移动count个单位
 * 为新元素腾出空间，保持数组内容的连续性
 *
 */
void shift_left(int start, int count) {
  for (int j = start; j < nr_token; ++j) {
    tokens[j - count] = tokens[j];
  }
  memset(&tokens[nr_token - 1], 0, sizeof(tokens[nr_token - 1]));
  nr_token -= count;
}

/**
 * Use unary operator, direct 1 -> -1
 */
int handle_neg(int i) {
    bool isUnary = (i == 0 || tokens[i - 1].type != TK_NUM);

    if (isUnary && tokens[i + 1].type == TK_NUM) {
        // Unary operator
        int num_value = 0;
        if (sscanf(tokens[i + 1].str, "%d", &num_value) != 1) {
            fprintf(stderr, "Error converting string to number: %s\n", tokens[i + 1].str);
            exit(EXIT_FAILURE);
        }
        tokens[i + 1].num_value = -num_value;
        shift_left(i + 1, 1);
        i++;
        return i;
    } 
    // Binary operator
    return i;
}

bool hex_judge(int i) {
    if (tokens[i].type != TK_NUM) {
        return false;;
    }
    if (tokens[i].str[0] == '0' && (tokens[i].str[1] == 'x' || tokens[i].str[1] == 'X')) {
        return true;
    }
    return false;
}

void hex_reader(int i) {
    if (tokens[i].type == TK_NUM
        && sscanf(tokens[i].str, "%x", &tokens[i].num_value) != 1) {
        fprintf(stderr, "Error converting hexadecimal string to number: %s\n", tokens[i].str);
        exit(EXIT_FAILURE);
    }
}

void dec_reader(int i) {
    if (tokens[i].type == TK_NUM
        && sscanf(tokens[i].str, "%d", &tokens[i].num_value) != 1) {
        fprintf(stderr, "Error converting string to number: %s\n", tokens[i].str);
        exit(EXIT_FAILURE);
    }
}
/**
 * numbers in expressions --> numbers
 */
void str2num(int p, int q) {
    for (int i = p; i < q; ++i) {
        if (hex_judge(i)) {
            hex_reader(i);
        } else {
            dec_reader(i);
        }

        if (tokens[i].type == TK_NEG) {
            i = handle_neg(i);
        }
    }
}

void is_deref() {
    // mul or deref
    // TODO: 不一定是NUM？REG也可以？
    for (int i = 0; i < nr_token; i ++) {
        if (tokens[i].type == TK_MUL
            && (i == 0 || tokens[i - 1].type != TK_NUM) ) {
            tokens[i].type = TK_DEREF;
        }
    }
}

void handle_pointer(int p, int q) {
    for (int i = p; i < q; i++) {
        if (tokens[i].type == TK_DEREF) {
            // *0x80000000 ---> 663 / 02 73;
            word_t addr = tokens[i + 1].num_value; 
            tokens[i + 1].num_value = vaddr_read((vaddr_t)addr, 4);
            shift_left(i + 1, 1);
        }
    }
}

void tokens_pre_processing(void) {
    is_neg();

    is_deref();
    
    str2num(0, nr_token);
 
    handle_pointer(0, nr_token);
}

int expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  tokens_pre_processing();

  /* TODO: Insert codes to evaluate the expression. */
  int result = eval(0, nr_token - 1);
  // 赋值范围问题，隐式转换，解决?

  if (result == INT_MAX) {
    *success = false;
    return 0;
  }

  return result;
}
