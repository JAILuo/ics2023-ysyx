#include <common.h>
#include <isa.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_ITRACE_BUF 16

typedef struct {
    vaddr_t pc;
    uint32_t inst;
}iringbuf;

iringbuf itrace_buf[MAX_ITRACE_BUF + 1];
int cur_inst;
bool full = false;

void trace_inst(vaddr_t pc, uint32_t inst) {
    itrace_buf[cur_inst].pc = pc;
    itrace_buf[cur_inst].inst = inst;
    cur_inst++;
    if (cur_inst > MAX_ITRACE_BUF) {
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
    for (i = 0; i < MAX_ITRACE_BUF; i++) { 
        p = buf;
        if (cpu.pc == itrace_buf[i].pc) {
            printf(ANSI_FG_RED);
        }
        p += sprintf(buf, "buf[%d] %s" FMT_WORD ": %08x\t", i, 
                cpu.pc == itrace_buf[i].pc ? " --> " : "     ",
                itrace_buf[i].pc,
                itrace_buf[i].inst);
        disassemble(p, sizeof(buf), itrace_buf[i].pc, (uint8_t *)&itrace_buf[i].inst, 4);
        puts(buf);
        printf(ANSI_NONE);
    }
}

void display_pread(paddr_t addr, int len) {
    // load
    printf("pread at " FMT_PADDR " len=%d\n", addr, len);
}

void display_pwrite(paddr_t addr, int len, word_t data) {
    // store
    printf("pwrite at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", addr, len, data);
}

