#include <macro.h>
#include <device/map.h>
#include <utils.h>
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
#include <device/map.h>


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

IFDEF(CONFIG_ITRACE, {
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
})


// ----------- mtrace -----------
void display_pread(paddr_t addr, int len) {
    // load
    log_write("[mtrace] " "pread at " FMT_PADDR " len=%d\n", addr, len);
}

void display_pwrite(paddr_t addr, int len, word_t data) {
    // store
    log_write("[mtrace] " "pwrite at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", addr, len, data);
}


// ----------- parse_elf -----------
/*
typedef MUXDEF(CONFIG_ISA64, Elf64_Ehdr, Elf32_Ehdr) Elf_Ehdr;
typedef MUXDEF(CONFIG_ISA64, Elf64_Phdr, Elf32_Phdr) Elf_Phdr;
typedef MUXDEF(CONFIG_ISA64, Elf64_Shdr, Elf32_Shdr) Elf_Shdr;
typedef MUXDEF(CONFIG_ISA64, Elf64_Sym , Elf64_Sym) Elf_Sym;
typedef MUXDEF(CONFIG_ISA64, Elf64_Rel , Elf32_Rel) Elf_Rel;
typedef MUXDEF(CONFIG_ISA64, Elf64_Rela , Elf32_Rela) Elf_Rela;
typedef MUXDEF(CONFIG_ISA64, Elf64_Dyn, Elf32_Dyn) Elf_Dyn;
typedef MUXDEF(CONFIG_ISA64, Elf64_Nhdr, Elf32_Nhdr) Elf_Nhdr;
*/

static Elf32_Ehdr eh;
ftrace_entry *ftrace_tab = NULL;
int ftrace_table_size = 0;

int call_depth = 0;

/*
TailRecNode *tail_rec_head = NULL; // linklist with head, dynamic allocated

static void init_tail_rec_list() {
	tail_rec_head = (TailRecNode *)malloc(sizeof(TailRecNode));
	tail_rec_head->pc = 0;
	tail_rec_head->next = NULL;
}

TailRecNode *node = NULL;
static void insert_tail_rec(paddr_t pc, int depth) {
    node = (TailRecNode *)malloc(sizeof(TailRecNode));
	node->pc = pc;
	node->depth = depth;
	node->next = tail_rec_head->next;
	tail_rec_head->next = node;
}

static void remove_tail_rec() {
	TailRecNode *node = tail_rec_head->next;
	tail_rec_head->next = node->next;
	free(node);
}
*/
/* 
 * the tail of recusive optimization code above comes from Internet
 * TODO
 **/


static void parse_elf_header(int fd, const char *elf_file) {
    Assert(lseek(fd, 0, SEEK_SET) == 0, "elf_file: %s lseek error.", elf_file);
    Assert(read(fd, (void *)&eh, sizeof(Elf32_Ehdr)) == sizeof(Elf32_Ehdr),
                "elf_file: %s lseek error.", elf_file);
 
    // 结束符号怎么表示？
    if (strncmp((const char *)eh.e_ident, "\177ELF", 4) != 0)
        panic("elf_file format error");
}

static void get_section_header(int fd, Elf32_Shdr *sh_tab) {
    //printf("e_shoff:%d\n", eh.e_shoff);
    //printf("e_ehsize:%u\n", eh.e_ehsize);
    /* didn't notice the bit of ELf  */
    lseek(fd, eh.e_shoff, SEEK_SET);
    Assert(lseek(fd, eh.e_shoff, SEEK_SET) == eh.e_shoff,
                "section header offset error.");

    for (int i = 0; i < eh.e_shnum; i++ ) {
        Assert(read(fd, (void*)&sh_tab[i], eh.e_shentsize) == eh.e_shentsize,
                    "section header size error.");
    }
}

static void read_section(int fd, Elf32_Shdr sh, void *dst) {
	assert(dst != NULL);
	assert(lseek(fd, (off_t)sh.sh_offset, SEEK_SET) == (off_t)sh.sh_offset);
	assert(read(fd, dst, sh.sh_size) == sh.sh_size);
}

static void iterate_symbol_table(int fd, Elf32_Shdr *sh_tab, int sym_index) {
    /* symbol table data */
    Elf32_Sym sym_tab[sh_tab[sym_index].sh_size];
    int sym_num = sh_tab[sym_index].sh_size / sizeof(Elf32_Sym);
    read_section(fd, sh_tab[sym_index], sym_tab);

    /* string table data:  */
    int str_index = sh_tab[sym_index].sh_link;
    char str_tab[sh_tab[str_index].sh_size];
    read_section(fd, sh_tab[str_index], str_tab);

    /* store the ftrace */
    /* future optimize: just store type that is FUNC. */
    ftrace_tab = (ftrace_entry *)malloc(sym_num * sizeof(ftrace_entry));
    ftrace_table_size = sym_num;
    // printf("sym_num or ftrace_table_size: %d\n", ftrace_table_size);
    for (int i = 0; i < ftrace_table_size; i++) {
        ftrace_tab[i].addr = sym_tab[i].st_value;
        ftrace_tab[i].info = sym_tab[i].st_info;
        ftrace_tab[i].size = sym_tab[i].st_size;
        memset(ftrace_tab[i].name, '\0', 32);
        strncpy(ftrace_tab[i].name, str_tab + sym_tab[i].st_name, 31);
        ftrace_tab[i].name[31] = '\0';
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
        if (ELF32_ST_TYPE(ftrace_tab[i].info) == STT_FUNC) {
            if (is_call) {
                if (ftrace_tab[i].addr == target) break;
            } else {
                if (ftrace_tab[i].addr <= target
                    && target < ftrace_tab[i].addr + ftrace_tab[i].size) break;
            }
        }
    }
    return i < ftrace_table_size ? i : -1;
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

   //init_tail_rec_list();

   close(fd);
}

// ----------- ftrace -----------
#define CALL_DEPTH (((call_depth) * (2)) % 50) 
void ftrace_func_call(vaddr_t pc, vaddr_t target, bool is_tail) {
    if (!ftrace_tab) return;

    call_depth++;
    int i = find_symbol_func(target, true); 
    printf(FMT_WORD ":%*scall [%s@" FMT_WORD "]\n",
    //log_write(FMT_WORD ":%*scall [%s@" FMT_WORD "]\n",
           pc,
           CALL_DEPTH, " ",
           //call_depth, " ",
           i >= 0 ? ftrace_tab[i].name : "???",
           target);
    /*
    if (is_tail) {
		insert_tail_rec(pc, call_depth-1);
    }
    */
}

void ftrace_func_ret(vaddr_t pc) {
    if (!ftrace_tab) return;

   int i = find_symbol_func(pc, false);
   printf(FMT_WORD ":%*sret [%s]\n",
   //log_write(FMT_WORD ":%*sret [%s]\n",
           pc,
           CALL_DEPTH, " ",
           //call_depth, " ",
           i >=0 ? ftrace_tab[i].name : "???");
    call_depth--;

    /*
    TailRecNode *node = tail_rec_head->next;
	if (node != NULL) {
		if (node->depth == call_depth) {
			paddr_t ret_target = node->pc;
			remove_tail_rec();
			ftrace_func_ret(ret_target);
		}
	}
    */
}


// ----------- dtrace -----------
/*
void trace_dread(paddr_t addr, int len, IOMap *map) {
    printf("dtrace: read %10s at " FMT_WORD "%d\n", map->name, addr, len);
}

void trace_dwrite(paddr_t addr, int len, word_t data, IOMap *map) {
    printf("dtrace: write %10s at " FMT_WORD "%d with " FMT_WORD,
           map->name, addr, len, data);
}
*/

// ----------- dtrace -----------
void etrace_log() {
    log_write("[etrace] " "exception NO: %d\n"
           "epc: %x\n", cpu.csr.mcause, cpu.csr.mepc);
}



