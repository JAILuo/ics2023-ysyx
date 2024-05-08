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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <assert.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

enum {
  TK_NOTYPE = 256,
  TK_EQ = 257,

  TK_ADD = 258,
  TK_SUB = 259,
  TK_MUL = 260,
  TK_DIV = 261,

  TK_RIGHT_PAR = 262,
  TK_LEFT_PAR = 263,

  TK_NUM = 264,
  TK_WORD = 265,

  TK_NEG = 266,

  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {"\\(", TK_LEFT_PAR},
    {"\\)", TK_RIGHT_PAR},

    // numbers (确保是+号，表示一个或多个数字)
    {"[0-9]+", TK_NUM},
    // words (确保在数字之后，这样数字不会被当作单词匹配)
    {"[A-Za-z_][A-Za-z0-9_]*", TK_WORD},

    {" +", TK_NOTYPE}, // spaces
    {"==", TK_EQ},     // equal

    {"\\+", TK_ADD}, // plus
    {"-", TK_SUB},
    {"\\*", TK_MUL},
    {"/", TK_DIV},

    //      好像我下面这么写不行？
    //    {"\\d", TK_NUM},
    //    {"\\w", TK_WORD},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

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
  char str[32];
  int num_value;
} Token;

static Token tokens[32] __attribute__((used)) = {};
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

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
        case TK_NOTYPE:
          /* 直接跳过空格 */
          break;

        case TK_ADD:
          // tokens[i] = {.type = TK_ADD};
          tokens[nr_token++].type = TK_ADD;
          break;

        case TK_SUB: // sub and negative.
          tokens[nr_token++].type = TK_SUB;

          break;
        case TK_MUL:
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
          // tokens[token_index].num_value = atoi(substr_start);
          nr_token++;
          break;

        case TK_WORD:
          tokens[nr_token++].type = TK_WORD;
          break;

        case TK_RIGHT_PAR:
          tokens[nr_token++].type = TK_RIGHT_PAR;
          break;

        case TK_LEFT_PAR:
          tokens[nr_token++].type = TK_LEFT_PAR;
          break;

        default:
          printf("No rules were mathced with %s\n", substr_start);
        }

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

/**
 * version1.2
 * (70)-(52) 不行
 *
 * 既然已经过了第一部分的检查,
 *       那我直接从 p + 1 开始判断到 q -1
 *       这样子不就直接跳过了吗?
 */
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

int get_priority(int op_type) {
  switch (op_type) {
  case TK_ADD:
  case TK_SUB:
    return 1;
  case TK_MUL:
  case TK_DIV:
    return 2;
  default:
    return 0; // Default priority for non-operators
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
        } else if (tokens[i].type >= TK_ADD && tokens[i].type <= TK_DIV) {
            // 只有在不在括号内时，才检查运算符
            if (in_parens == 0) {
                int priority = get_priority(tokens[i].type);
                if (priority < *min_priority) {
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

word_t compute(int op_type, int val1, int val2) {
  // op = the position of 主运算符 in the token expression;
  // 下面是最底层括号内的运算：
  switch (op_type) {
  case TK_ADD:
    return val1 + val2;
    break;
  case TK_SUB: /* ... */
    return val1 - val2;
    break;
  case TK_MUL: /* ... */
    return val1 * val2;
    break;
  case TK_DIV:
    if (val2 == 0) {
      printf("error:division not 0.\n");
      return -1;
    }
    return val1 / val2;
    break;
  default:
    assert(0);
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
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type != TK_NUM) {
      printf("error_eval, expect number but got %d\n", tokens[p].type);
      return INT_MAX;
    }
    return tokens[p].num_value;
  } else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  } else {
    // int flag = 0;
    int op = -1;
    int min_priority = INT_MAX;

    op = find_main_operator(p, q, &min_priority);
    if (op == -1) {
      printf("No operator found.\n");
      return INT_MAX;
    }

    op_type = tokens[op].type;

    word_t val1, val2;
    val1 = eval(p, op - 1);
    val2 = eval(op + 1, q);
    return compute(op_type, val1, val2);
  }
}

/**
 * 检查并转换负号标记。
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
 */
void shift_left(int start, int count) {
  for (int j = start; j < nr_token; ++j) {
    tokens[j - count] = tokens[j];
  }
  // 为什么要覆盖这个？我不是要覆盖最后一个吗？
  //memset(&tokens[start + count], 0, sizeof(tokens[start + count]));
  memset(&tokens[nr_token - 1], 0, sizeof(tokens[nr_token - 1]));
  nr_token -= count;
}

/**
 * 处理连续的负号和开头的负号。
 *
 * @note 处理连续的负号和开头的负号
 * @note i + 2 出现这么多次?
 * @note eg. 2--1 :第一个减号为i
 */
int handle_neg(int i) {
    if (tokens[0].type == TK_NEG // 开头负号：-1+2
               && (tokens[i + 1].type == TK_NUM))
    {
        tokens[i + 1].num_value = -atoi(tokens[i + 1].str);
        shift_left(i + 1 , 1);
        i++;
        // 但是我这么处理好像处理不了--1?
        // 再加一个if?
    }else if (tokens[0].type == TK_NEG 
              && tokens[1].type == TK_NEG) {
        tokens[i + 2].num_value = -atoi(tokens[i + 2].str);
        tokens[i] = tokens[i + 2];
        memset(&tokens[1], 0, sizeof(tokens[1]));
        memset(&tokens[2], 0, sizeof(tokens[2])); 
        nr_token-=2;
        i += 2;
    } else if (tokens[i + 1].type == TK_NEG
               && tokens[i + 2].type == TK_NUM) {
        switch (tokens[i].type) { 
            case TK_LEFT_PAR: //这里的逻辑怎么实现？
                tokens[i + 2].num_value = -atoi(tokens[i + 2].str);
                shift_left(i + 2, 1);
                i++;
                break;
            case TK_RIGHT_PAR: //这里的逻辑怎么实现？
                tokens[i + 1].type = TK_ADD;
                tokens[i + 2].num_value = -atoi(tokens[i + 2].str);
                i+=2;
                break;

            case TK_ADD:
                tokens[i + 2].num_value = -atoi(tokens[i + 2].str);
                shift_left(i + 2, 1);
                i++;
                break;
            
            case TK_SUB:
                tokens[i].type = TK_ADD;

                tokens[i + 2].num_value = atoi(tokens[i + 2].str);
                shift_left(i + 2, 1);
                i += 2;
                break;

            case TK_MUL:
                tokens[i].type = TK_MUL;

                tokens[i + 2].num_value = -atoi(tokens[i + 2].str);
                shift_left(i + 2, 1);
                i++;
                break;
        
            case TK_DIV:
                tokens[i].type = TK_DIV;

                tokens[i + 2].num_value = -atoi(tokens[i + 2].str);
                shift_left(i + 2, 1);
                i++;
                break;
        }
    }
    return i;
}

/**
 * 将字符串表达式中的数字转换为数值，
 */
void string_2_num(int p, int q) {
    // 使用atoi? 还是strtoul? 转换字符串为数字
    for (int i = p; i < q; ++i) {
        // 正常正数
        if (tokens[i].type == TK_NUM) {
            tokens[i].num_value = atoi(tokens[i].str);
        }
        // 负数处理
        i = handle_neg(i);
    }
}

void tokens_pre_processing(void) {
    is_neg();

    string_2_num(0, nr_token);
}

int expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  tokens_pre_processing();

  /* TODO: Insert codes to evaluate the expression. */
  int result = eval(0, nr_token - 1);
  // 赋值范围问题，隐式转换，等会解决
  // 上面的避免使用atoi，也是一个好习惯，因为会整数溢出

  // 默认结果都是uint32_t,负数先不管
  if (result == INT_MAX) {
    *success = false;
    return 0;
  }

  return result;
}
