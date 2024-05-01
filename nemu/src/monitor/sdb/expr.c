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

#include <bits/types/timer_t.h>
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

enum {
    TK_NOTYPE = 256,
    TK_EQ,

    TK_ADD,
    TK_SUB,
    TK_MUL,
    TK_DIV,

    TK_RIGHT_PAR,
    TK_LEFT_PAR,

    TK_NUM,
    TK_WORD,

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

    {" +", TK_NOTYPE},      // spaces
    {"==", TK_EQ},          // equal
    
    {"\\+", TK_ADD},        // plus
    {"-", TK_SUB},
    {"\\*", TK_MUL},
    {"/", TK_DIV},

    
//      好像我下面这么写不行？    
//    {"\\d", TK_NUM},
//    {"\\w", TK_WORD},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
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
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

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

            case TK_SUB:
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
                //tokens[token_index].num_value = atoi(substr_start);
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
/*
static bool check_parentheses(int p, int q) {
    int parentheses = 0;

    if (tokens[p].type != TK_LEFT_PAR || tokens[p].type != TK_RIGHT_PAR) {
        return false;
    }

    for (int i = p; i <= q; i++) {
        if (tokens[i].str[0] == '(' ) {
            parentheses++;
        }
        if (tokens[i].str[0] == ')' ) {
            parentheses--;
        }
        if (parentheses < 0) {
            return false;
        }
    }
    return parentheses == 0;
}
*/

bool check_parentheses(int p, int q)
{
    if(tokens[p].type != TK_LEFT_PAR || tokens[q].type != TK_RIGHT_PAR) {
        return false;
    }

    int l = p , r = q;
    while(l < r)
    {
        if(tokens[l].type == '('){
            if(tokens[r].type == ')')
            {
                l ++ , r --;
                continue;
            }

            else
                r --;
        }
        else if(tokens[l].type == ')')
            return false;
        else l ++;
    }
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

/* 
 * 在token表达式中指示一个子表达式, 
 * 使用两个整数 p 和 q 来指示这个子表达式的开始位置和结束位置. 
 */
int eval(int p, int q) {
    int op_type;
    if (p > q) {
    /* Bad expression */
        printf("error_eval.\n");
        return -1;
    } else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
        if (tokens[p].type == TK_NUM) {
            return tokens[p].num_value;
        } else {
            printf("No num, error.\n");
            return -1;
        }

    } else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
        return eval(p + 1, q - 1);
    } else {
        int op = -1;
        int min_priority = INT_MAX;

        for (int i = p; i <= q; i++) {
            if (tokens[i].type == TK_LEFT_PAR) {
                int par_right = i;
                while (tokens[par_right].type != TK_RIGHT_PAR) {
                    /* 跳过括号内的表达式，直到遇到对应的右括号 */
                    par_right++;
                }

                /* 2024.04.30 晚上
                 * 现在的bug：1. 空格识别 2. 子表达式的括号匹配
                 * 已解决：1. 不把空格放到tokens里
                 *         2. gdb出来发现自己写的判断括号算法有问题
                 *            改用网上的双指针算法，不会，留着待会学
                 * 现在出现新问题：4 +3*(2- 1) 直接错误退出了。
                 */
                if(check_parentheses(i, par_right) == false) {
                    printf("parentheses in sub expression was not matched.\n");
                    return -1;
                }
                
            }
            if (tokens[i].type >= TK_ADD && tokens[i].type <= TK_DIV) {
                int priority = get_priority(tokens[i].type);
                if (priority <= min_priority) {
                    min_priority = priority;
                    op = i;
                    op_type = tokens[i].type;
                }
            }
        }

        if (op == -1) {
            printf("No operator found.\n");
            return -1;
        }

        // op = the position of 主运算符 in the token expression;
        
        int val1 = eval(p, op - 1);
        int val2 = eval(op + 1, q);

        switch (op_type) {
            case TK_ADD: 
                return val1 + val2;
                break;
            case TK_SUB: /* ... */
                return val1 - val2;
                break;
            case TK_MUL: /* ... */
                return val1 *val2;
                break;
            case TK_DIV: /* 先不做各种防御性检查，比如除零 */
                return val1 / val2;
                break;
            //case 4:
            //    return val1 == val2;
            //case 5:
            //    return val1 != val2;
            //case 6:
            //    return val1 || val2;
            //case 7:
            //    return val1 && val2;
            default: assert(0);
        }
    }
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // 确保使用strtol转换字符串为数字
  for (int i = 0; i < nr_token; ++i) {
      if (tokens[i].type == TK_NUM) {
          tokens[i].num_value = atoi(tokens[i].str);
      }
  }

  /* TODO: Insert codes to evaluate the expression. */
  int result = (word_t)eval(0, nr_token - 1);
  if (result == -1) {
    *success = false;
  }

  return result;
}
