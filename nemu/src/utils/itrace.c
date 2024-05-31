#include "utils.h"
#include <common.h>
#include <isa.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// ----------- itrace -----------
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


// ----------- mtrace -----------
void display_pread(paddr_t addr, int len) {
    // load
    printf("pread at " FMT_PADDR " len=%d\n", addr, len);
}

void display_pwrite(paddr_t addr, int len, word_t data) {
    // store
    printf("pwrite at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", addr, len, data);
}


// ----------- ftrace -----------
static Elf32_Ehdr eh;
ftrace_entry *ftrace_tab = NULL;
int ftrace_table_size = 0;

int call_depth = 0;

static void parse_elf_header(int fd, const char *elf_file) {
    Assert(lseek(fd, 0, SEEK_SET) == 0, "elf_file: %s lseek error.", elf_file);
    Assert(read(fd, (void *)&eh, sizeof(Elf32_Ehdr)) == sizeof(Elf32_Ehdr),
                "elf_file: %s lseek error.", elf_file);
 
    // 结束符号怎么表示？
    if (strncmp((const char *)eh.e_ident, "\177ELF", 4) != 0) {
        panic("elf_file format error");
    }
}

static void get_section_header(int fd, Elf32_Shdr *sh_tab) {
    //printf("e_shoff:%d\n", eh.e_shoff);
    //printf("e_ehsize:%u\n", eh.e_ehsize);
    lseek(fd, eh.e_shoff, SEEK_SET);
    Assert(lseek(fd, eh.e_shoff, SEEK_SET) == eh.e_shoff,
                "section header offset error.");

    printf("%d\n", eh.e_shnum);
    for (int i = 0; i < eh.e_shnum; i++ ) {
        Assert(read(fd, (void*)&sh_tab[i], eh.e_shentsize) == eh.e_shentsize,
                    "section header size error.");
    }


    // 一行行读到这段表中,应该说逐个section header读取
}

static void read_section(int fd, Elf32_Shdr sh, void *dst) {
	assert(dst != NULL);
	assert(lseek(fd, (off_t)sh.sh_offset, SEEK_SET) == (off_t)sh.sh_offset);
	assert(read(fd, dst, sh.sh_size) == sh.sh_size);
}

static void iterate_symbol_table(int fd, Elf32_Shdr *sh_tab, int sym_index) {
    // 从段表中读取符号表的内容,存到sym_tab
    Elf32_Sym sym_tab[sh_tab[sym_index].sh_size];
    int sym_num = sh_tab[sym_index].sh_size / sizeof(Elf32_Sym);
    // 这64忘记改为32了，导致解析出来的符号个数错误
    read_section(fd, sh_tab[sym_index], sym_tab);

    int str_index = sh_tab[sym_index].sh_link;
    char str_tab[sh_tab[str_index].sh_size];
    read_section(fd, sh_tab[str_index], str_tab);

    ftrace_tab = (ftrace_entry *)malloc(sym_num * sizeof(ftrace_entry));
    ftrace_table_size = sym_num;
    printf("sym_num/ftrace_table_size: %d\n", ftrace_table_size);
    for (int i = 0; i < ftrace_table_size; i++) {
        ftrace_tab[i].addr = sym_tab[i].st_value;
        ftrace_tab[i].info = sym_tab[i].st_info;
        ftrace_tab[i].size = sym_tab[i].st_size;
        memset(ftrace_tab[i].name, '\0', 32);
        strncpy(ftrace_tab[i].name, str_tab + sym_tab[i].st_name, 31);
        ftrace_tab[i].name[31] = '\0';

        // 有几个名字确实是读出来了。
        printf("ftrace_tab[%d]:\n",i);
        printf(FMT_WORD"\n", ftrace_tab[i].addr);
        printf("name: %s\n", ftrace_tab[i].name);
        printf("info: %c\n\n", ftrace_tab[i].info);
    }
}
static void get_symbols(int fd, Elf32_Shdr *sh_tab) {
    for (int i = 0; i < eh.e_shnum; i++) {
        switch (sh_tab[i].sh_type) {
            case SHT_SYMTAB: case SHT_DYNSYM:
                iterate_symbol_table(fd, sh_tab, i);
        }
    }
}

int find_symbol_func(vaddr_t target, bool is_call) {
    int i = 0;
    for (; i < ftrace_table_size; i++) {
        //if (ftrace_tab[i].info == STT_FUNC) {
        if (ELF32_ST_TYPE(ftrace_tab[i].info) == STT_FUNC) {
            // 如果我这里找到了对应传入的目标地址，那我返回真？
            // 那要是返回指令呢？
            // 这里还得进行判断是返回指令还是函数调用指令，加一个bool
            if (is_call) {
                if (ftrace_tab[i].addr == target) break;
            } else {
                //if (ftrace_tab[i].addr <= target) break;
                if (ftrace_tab[i].addr <= target && target < ftrace_tab[i].addr + ftrace_tab[i].size) break;
            }
        }
    }
    return i;
}

void parse_elf(const char *elf_file) {
   if (!elf_file) {
       return;
   }

   printf("elf_file:%s\n", elf_file);
   int fd = open(elf_file, O_RDONLY|O_SYNC);
   Assert(fd != -1, "elf_file: %s open error", elf_file);

   parse_elf_header(fd, elf_file);

   Elf32_Shdr sh[eh.e_shentsize * eh.e_shnum];
   get_section_header(fd, sh);

   get_symbols(fd, sh);
}

void ftrace_func_call(vaddr_t pc, vaddr_t target) {
    if (!ftrace_tab) return;

    call_depth++;
    
    int i = find_symbol_func(target, true); 
    if (i < 0) {
        printf("no func matched symbol find.\n");
        return;
    }

    printf(FMT_WORD ":%*scall [%s@" FMT_WORD "]\n",
           pc,
           call_depth * 2, "",
           ftrace_tab[i].name,
           target);
    // 注意，我在main.c的最后进行了free处理，之后记得改
    // 另外，在看各种call和ret指令的时候，包括保存返回地址和不保存返回地址。
}

void ftrace_func_ret(vaddr_t pc) {
    if (!ftrace_tab) return;

    int i = find_symbol_func(pc, false);
    printf(FMT_WORD ": %*sret [%s]\n",
           pc,
           call_depth * 2, "",
           ftrace_tab[i].name);

    call_depth--;

}

