#include <proc.h>
#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t get_ramdisk_size();

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
# define EXPECT_TYPE EM_386
#elif defined(__ISA_MIPS32__)
# define EXPECT_TYPE
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_LOONGARCH32R__)
# define EXPECT_TYPE
#elif
# error unsupported ISA __ISA__
#endif

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

static uintptr_t loader(PCB *pcb, const char *filename) {
    int fd = fs_open(filename, 0, 0);

    Elf_Ehdr eh;
    fs_read(fd, &eh, sizeof(Elf_Ehdr));
    assert(*(uint32_t *)eh.e_ident == 0x464c457f); //little endian 7f 45 4c 46

    // check machine  
    assert(eh.e_machine == EXPECT_TYPE);

    Elf_Phdr ph[eh.e_phnum];
    fs_lseek(fd, eh.e_phoff, SEEK_SET);
    fs_read(fd, ph, sizeof(Elf_Phdr) * eh.e_phnum);
    for (size_t i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {
            fs_lseek(fd, ph[i].p_offset, SEEK_SET);
            // vaddr or paddr？
            fs_read(fd, (void *)(uintptr_t)ph[i].p_vaddr, ph[i].p_memsz);
            memset((void *)(uintptr_t)ph[i].p_vaddr + ph[i].p_filesz, 0, 
                   ph[i].p_memsz - ph[i].p_filesz);
        }
    }

    fs_close(fd);
    return (uintptr_t)eh.e_entry;
}

//void naive_uload(PCB *pcb, const char *filename) {
void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
    Area stack = {
        .start = pcb->stack,
        .end = pcb->stack + STACK_SIZE
    };
    pcb->cp = kcontext(stack, entry, arg);
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
    uintptr_t entry = loader(pcb, filename);
    Area stack = {
        .start = pcb->stack,
        .end   = pcb->stack + STACK_SIZE
    };

    // kernel stack
    Log("process name: %s", filename);
    Log("entry: %p", entry);
    Log("stack.start: %p, stack.end: %p", stack.start, stack.end);

    // create user context
    pcb->cp = ucontext(&pcb->as, stack, (void *)entry);


    // new process user stack
    void *new_process_stack = new_page(8); // has beend aligined
    size_t space_count = 0;


    // begin base in ucontext(Contex structure below) ? some question 
    int envpc = 0;
    int argc = 0;
    while (envp != NULL && envp[envpc] != NULL) {
        envpc++; 
    }
    while (argv != NULL && argv[argc] != NULL) {
        argc++;
    }

    // uintptr_t for portability
    space_count += sizeof(uintptr_t); // store argc
    space_count += sizeof(uintptr_t) * (argc + 1); // store argv
    space_count += sizeof(uintptr_t) * (envpc + 1); // store envp
    for (int i = 0; i < envpc; ++i) { 
        space_count += (strlen((const char *)envp[i]) + 1);  // the length of each element in envp[]
    }
    for (int i = 0; i < argc; ++i) { 
        space_count += (strlen((const char *)argv[i]) + 1);  // the length of each element in envp[]
    }
    Log("envpc: %d, argc: %d, space_count: %d", envpc, argc, space_count);


    Log("Base before ROUNDUP:%p", new_process_stack - space_count);
    uintptr_t *base = (uintptr_t *)ROUNDDOWN(new_process_stack - space_count, sizeof(uintptr_t));
    uintptr_t *base_mem = base;
    Log("Base after ROUNDUP:%p", base);


    *base = argc;
    base++;


    base += (argc + 1) + (envpc + 1); // string area
    char *string_area = (char *)base;
    uintptr_t *string_base = (uintptr_t*)base;
    char *tmp_argv[argc];
    char *tmp_envp[envpc];
    for (int i = 0; i < argc; i++) {
        strcpy(string_area, argv[i]);
        printf("string_area: %s\n", string_area + i);
        printf("argv[%d]:%s\n", i, argv[i]);
        tmp_argv[i] = string_area;
        string_area += strlen((argv[i]) + 1);
    }

    for (int i = 0; i < envpc; i++) {
        strcpy(string_area, envp[i]);
        tmp_envp[i] = string_area;
        string_area += strlen((envp[i]) + 1);
    }


    base -= (argc + 1) + (envpc + 1);
    for (int i = 0; i < argc; i++, base++) {
        *base = (uintptr_t)tmp_argv[i];
    }
    *base = (uintptr_t)NULL; // argv[argc] = NULL
    base++;
    
    for (int i = 0; i < envpc; i++, base++) {
        *base = (uintptr_t)tmp_envp[i];
    }
    *base = (uintptr_t)NULL; // argv[envpc] = NULL
    base++;

    // Unspecified ...
    assert(string_base == base);

    pcb->cp->GPRx = (uintptr_t)base_mem;
}

