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
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NR_WP 32

/**
 * Imitate the GDB format
 */

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[32];
  int old_result;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
static int next_no = 0;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* Prepend or Append? */
static WP* new_wp() {
    Assert(free_, "No idle watchpoint.");

    WP *new_wp = free_;
    free_ = free_->next;
    
    new_wp->NO = next_no++;
    // Prepend
    new_wp->next = head;
    head = new_wp;

    return new_wp;
}

static WP *find_prev_wp(int no) {
  WP *cur = head;
  WP *prev = NULL;
  while (cur != NULL && cur->NO != no) {
    prev = cur;
    cur = cur->next;
  }
  return prev;
}

static void free_wp(WP *wp) {
    if (!wp) return;

    WP *prev = find_prev_wp(wp->NO);
    if (prev == NULL) { // wp is head node
        head = wp->next;
    } else {            // Updatae the next pointer of the prcursor node
        prev->next = wp->next;
    }
    
    // Add wp to the header of the idle watchpoint linked list
    wp->next = free_;
    free_ = wp;
}

void watch_wp(char *expr, int result) {
    WP *wp = new_wp();
    int len = strlen(expr);
    if (len > sizeof(wp->expr) - 1) {
        len = sizeof(wp->expr) - 1;
    }
    strncpy(wp->expr, expr,len);
    wp->expr[len] = '\0';

    wp->old_result = result;
    printf("watchpoint %d: %s\n", wp->NO, wp->expr);
}

void delete_wp(int no) {
    // 传入要删除的监视点的序号
    if (no < 0 || no >= NR_WP) {
        printf("Watchpoint number out of range.\n");
        return;
    }
    WP *cur = &wp_pool[no];

    free_wp(cur);
    printf("Delete watchpoint %d\n",cur->NO);
}

/**
 * 
 */
void diff_wp(void) {
    WP *cur = head;
    bool success = true;
    while (cur) {
        int new_result = expr(cur->expr, &success);
        if (new_result != cur->old_result) {
            printf("watchpoint %d: %s\n\n"
                   "Old value: %d\n"
                   "New value: %d\n",
                   cur->NO, cur->expr, cur->old_result, new_result);
            cur->old_result = new_result;
            nemu_state.state = NEMU_STOP;
            printf("Watchpoint %d triggered. Program paused.\n", cur->NO);
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
