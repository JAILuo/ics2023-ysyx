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

#include "common.h"
#include "debug.h"
#include "sdb.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[128];
  int old_result;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
/* Why not queue? */
/* Prepend or Append? */
static WP* new_wp() {
    Assert(free_, "No idle watchpoint.");

    WP *new_wp = free_;
    free_ = free_->next;

    // 将新分配的监视点添加到正在使用的链表的头部
    new_wp->next = head;
    head = new_wp;

    return new_wp;
}

static void free_wp(WP *wp) {
    WP* tmp = head;
    if (tmp == wp) {
        head = NULL;
    } else {
        while (tmp && tmp->next != wp) {
            tmp = tmp->next;
        }
        Assert(tmp, "wp is not in the watchpoint linked list.\n");
        tmp->next = wp->next;
    }
    wp->next = free_;
    free_ = wp;
}

void watch_wp(char *expr, int result) {
    WP *wp = new_wp();
    word_t len = strlen(expr);
    if (len >= sizeof(wp->expr)) {
        len= sizeof(wp->expr) - 1;
    }
    strncpy(wp->expr, expr, sizeof(strlen(expr)));
    wp->expr[len] = '\0';

    wp->old_result = result;
    printf("watchpoint %d: %s\n", wp->NO, wp->expr);
}

void delete_wp(int no) {
    // 传入要删除的监视点的序号
    if (no > NR_WP) {
        printf("Watchpoint too large.\n");
        return;
    }
    WP *cur = &wp_pool[no];
    free_wp(cur);
    printf("Delete watchpoint %d\n",cur->NO);
}

void difftest_wp(void) {
    WP *cur = head;
    bool success = true;
    while (cur) {
        int new_result = expr(cur->expr, &success);
        if (new_result != cur->old_result) {
            printf("watchpoint %d: %s\n"
                   "Old value: %d\n"
                   "New value: %d\n",
                   cur->NO, cur->expr, cur->old_result, new_result);
            cur->old_result = new_result;
        }
        cur = cur->next;
    }
}

void display_wp(void) {
    WP *cur = head;
    if (!cur) {
        printf("No watchpoint.\n");
        return;
    }
    printf("Num\tWhat\n");
    while (cur) {
        printf("%d\t%s\n", cur->NO, cur->expr);
        cur = cur->next;
    }
}
