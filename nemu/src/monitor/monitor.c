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

#include "cpu/cpu.h"
#include <isa.h>
#include <memory/paddr.h>
#include <elf.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm(const char *triple);

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
//  Log("Exercise: Please remove me in the source code and compile NEMU again.");
//  assert(0);
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;
static char *elf_file = NULL;
static Elf64_Ehdr eh;

static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"elf"      , required_argument, NULL, 'e'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode();                   break;
      case 'p': sscanf(optarg, "%d", &difftest_port);   break;
      case 'l': log_file = optarg;                      break;
      case 'd': diff_so_file = optarg;                  break;
      case  1 : img_file = optarg;                      return 0;
      case 'e': elf_file = optarg;                      break;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

static void parse_elf_header(int fd) {
    Assert(lseek(fd, 0, SEEK_SET) != -1, "elf_file: %s lseek error.", elf_file);
    Assert(read(fd, (void *)&eh, sizeof(Elf64_Ehdr)) == sizeof(Elf64_Ehdr),
                "elf_file: %s lseek error.", elf_file);

    if (strncmp((const char *)eh.e_ident,  "\x7F ELF", 4) != 0) {
        panic("elf_file format error");
    }
}

static void get_section_header(int fd, Elf64_Shdr *sh_tab) {
    Assert(lseek(fd, eh.e_shoff, SEEK_SET) == eh.e_shoff,
                "section header offset error.");

    for (int i = 0; i < eh.e_shnum; i++ ) {
        Assert(read(fd, &sh_tab[i], eh.e_shentsize) == eh.e_shentsize,
                    "section header size error.");
    }
    // 一行行读到这段表中,应该说逐个section header读取
}

static void read_section(int fd, Elf64_Shdr sh, void *dst) {
	assert(dst != NULL);
	assert(lseek(fd, (off_t)sh.sh_offset, SEEK_SET) == (off_t)sh.sh_offset);
	assert(read(fd, dst, sh.sh_size) == sh.sh_size);
}

static void iterate_symbol_table(int fd, Elf64_Shdr *sh_tab, int sym_index) {
    // 从段表中读取符号表的内容,存到sym_tab
    Elf64_Sym sym_tab[sh_tab[sym_index].sh_size];
    read_section(fd, sh_tab[sym_index], sym_tab);

    // 符号一共几个项, 有几行
    int sym_num = sh_tab[sym_index].sh_size / sizeof(Elf64_Sym);
    // 从符号表中（符号表也是一个段）读取type为FUNC的 symbol entry
    for (int i = 0; i < sym_num; i++) {
        if (sym_tab[i].st_info == STT_FUNC) {

        }
    }
    // 还有一个问题，我们现在还需要在段表中找到字符串表
    // 但是怎么获得在字符串表在段表中的索引呢？就向符号表的sym_index

}
static void get_symbols(int fd, Elf64_Shdr *sh_tab) {
    for (int i = 0; i < eh.e_shnum; i++) {
       if (sh_tab[i].sh_type == SHT_SYMTAB) {
           iterate_symbol_table(fd, sh_tab, i);
       }
    }
}

void parse_elf(void) {
   if (!elf_file) {
       return;
   }
   int fd = open(elf_file, O_RDONLY);
   Assert(fd != -1, "elf_file: %s open error", elf_file);

   parse_elf_header(fd);

   Elf64_Shdr sh[eh.e_shentsize * eh.e_shnum];
   get_section_header(fd, sh);

   get_symbols(fd, sh);
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

  parse_elf();

#ifndef CONFIG_ISA_loongarch32r
  IFDEF(CONFIG_ITRACE, init_disasm(
    MUXDEF(CONFIG_ISA_x86,     "i686",
    MUXDEF(CONFIG_ISA_mips32,  "mipsel",
    MUXDEF(CONFIG_ISA_riscv,
      MUXDEF(CONFIG_RV64,      "riscv64",
                               "riscv32"),
                               "bad"))) "-pc-linux-gnu"
  ));
#endif

  /* Display welcome message. */
  welcome();
}

#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
