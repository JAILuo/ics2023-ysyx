#include <common.h>
#include <isa.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
#define MAX_ITRACE_BUF 16

typedef struct {
    vaddr_t pc;
    uint32_t inst;
}iringbuf;

iringbuf itrace_buf[MAX_ITRACE_BUF];
int cur_inst;

void trace_inst(vaddr_t pc, uint32_t inst) {
    if (cur_inst >= MAX_ITRACE_BUF) {
        cur_inst = 0; 
    }
    itrace_buf[cur_inst].pc = pc;
    itrace_buf[cur_inst].inst = inst;
    cur_inst++;
}

void display_inst(vaddr_t error_pc) {
    int start = cur_inst == 0 ? MAX_ITRACE_BUF - 1 : cur_inst - 1;
    int end = cur_inst;

    // printf("Recently executed instructions:\n");
    for (int i = start; i != end; i = (i - 1 + MAX_ITRACE_BUF) % MAX_ITRACE_BUF) {
        bool isError = (itrace_buf[i].pc == error_pc);
        printf(isError ? ANSI_FG_RED : ANSI_FG_GREEN);
        printf("buf[%d] %s" FMT_WORD ": %08x\t", i,
               isError ? " --> " : "     ",
               itrace_buf[i].pc,
               itrace_buf[i].inst);
        char buf[128];
        void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
        disassemble(buf, sizeof(buf), itrace_buf[i].pc, (uint8_t *)&itrace_buf[i].inst, 4);
        puts(buf);
        printf(ANSI_NONE);
    }
}
*/

/*
#include <common.h>

#define MAX_IRINGBUF 16

typedef struct {
  word_t pc;
  uint32_t inst;
} ItraceNode;

ItraceNode iringbuf[MAX_IRINGBUF];
int p_cur = 0;
bool full = false;

void trace_inst(word_t pc, uint32_t inst) {
  iringbuf[p_cur].pc = pc;
  iringbuf[p_cur].inst = inst;
  p_cur = (p_cur + 1) % MAX_IRINGBUF;
  full = full || p_cur == 0;
}

void display_inst() {
  if (!full && !p_cur) return;

  int end = p_cur;
  int i = full?p_cur:0;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  char buf[128];
  char *p;
  do {
    p = buf;
    p += sprintf(buf, "buf[%d] %s" FMT_WORD ": %08x ", i,
                 (i+1)%MAX_IRINGBUF==end?" --> ":"     ",
                 iringbuf[i].pc,
                 iringbuf[i].inst);
    disassemble(p, buf - p + sizeof(buf), iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

    if ((i+1)%MAX_IRINGBUF==end) printf(ANSI_FG_RED);
    puts(buf);
  } while ((i = (i+1)%MAX_IRINGBUF) != end);
  puts(ANSI_NONE);
}
*/
#define MAX_ITRACE 16

typedef struct {
    vaddr_t pc;
    uint32_t inst;
}iringbuf;

iringbuf itrace_buf[MAX_ITRACE];
int cur_inst;
bool full = false;

void trace_inst(vaddr_t pc, uint32_t inst) {
    itrace_buf[cur_inst].pc = pc;
    itrace_buf[cur_inst].inst = inst;
    cur_inst++;
    if (cur_inst > MAX_ITRACE) {
        full = true;
        cur_inst = 0;
    }
}

void display_inst() {    
    if (cur_inst == 0) {
        return;
    }
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    char buf[128];
    char *p;
    int i = full ? cur_inst:0;

    // puts("Recently executed instructions:");
    for (i = 0; i < MAX_ITRACE; i++) { 
        p = buf;
        if (cpu.pc == itrace_buf[i].pc) {
            printf(ANSI_FG_RED);
        }
        p += sprintf(buf, "buf[%d] %s" FMT_WORD ": %08x\t", i, 
                cpu.pc == itrace_buf[i].pc ? " --> " : "     ",
                itrace_buf[i].pc,
                itrace_buf[i].inst);
        disassemble(p, buf + sizeof(buf) - p, itrace_buf[i].pc, (uint8_t *)&itrace_buf[i].inst, 4);
        puts(buf);
        printf(ANSI_NONE);
    }
}

