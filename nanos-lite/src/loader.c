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
char magic[16] = {0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
char magic[16] = {0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
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

    // check magic number
    for (int i = 0; i < 16; i++) assert(magic[i] == eh.e_ident[i]);

    // check machine  
    assert(eh.e_machine == EXPECT_TYPE);

    Elf_Phdr *ph = (Elf_Phdr *)malloc(sizeof(Elf_Phdr) * eh.e_phnum);
    fs_lseek(fd, eh.e_phoff, SEEK_SET);
    fs_read(fd, ph, sizeof(Elf_Phdr) * eh.e_phnum);
    for (size_t i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {
            char *buf_malloc = (char *)malloc(ph[i].p_filesz * sizeof(char) + 1);
            fs_lseek(fd, ph[i].p_offset, SEEK_SET);

            fs_read(fd, (void *)buf_malloc, ph[i].p_memsz);// vaddr or paddr？
            
            //printf("in loader1, filename:%s\n", filename);

            memcpy((void *)(uintptr_t)ph[i].p_vaddr, buf_malloc, ph[i].p_filesz);
            
            //printf("in loader2, filename:%s\n", filename);
            memset((void *)(uintptr_t)ph[i].p_vaddr + ph[i].p_filesz, 0, 
                   ph[i].p_memsz - ph[i].p_filesz);

            free(buf_malloc);
            buf_malloc = NULL;
        }
    }
    free(ph);
    ph = NULL;

    fs_close(fd);
    return (uintptr_t)eh.e_entry;
}

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
    /*
      Log("kernel context: %p (a0=%p, sp=%p)",
      pcb->cp,
      (void *)pcb->cp->GPRx,
      (void *)pcb->cp->gpr[2]);
      */
}

static int test = 1;
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
    printf("name:%s\n", filename);
    if (test++ == 2) {
        for (int i = 0; i < 2; i++) {
            printf("argv[%d]: %s\n", i, argv[i]);
        }
    }
    uintptr_t entry = loader(pcb, filename);
    // create new kernel stack for new user process
    Area stack = {
        .start = pcb->stack,
        .end   = pcb->stack + STACK_SIZE
    };

    Log("process name: %s", filename);
    Log("entry: %p", entry);
    Log("stack.start: %p, stack.end: %p", stack.start, stack.end);

    // create new context for new process
    pcb->cp = ucontext(&pcb->as, stack, (void(*)())entry);

    // create new user stack for new user process
    void *new_user_stack = new_page(8); // has beend aligined

    // store argc, argv, envp in the user stack after ucontext.
    int envpc = 0, argc = 0;
    size_t space_count = 0;
    while (envp != NULL && envp[envpc] != NULL) envpc++; 
    while (argv != NULL && argv[argc] != NULL) argc++;

    // uintptr_t for portability
    space_count += sizeof(int); // store argc
    //Log("argc spce_count: %d", space_count);
    space_count += sizeof(uintptr_t) * (argc + 1); // store argv
    //Log("argv spce_count: %d", space_count);
    space_count += sizeof(uintptr_t) * (envpc + 1); // store envp
    //Log("envp spce_count: %d", space_count);
        printf("bugjhewr.\n");
    for (int i = 0; i < envpc; i++) { 
        //Log("envp spce_count: %d , envp[%d] len = %d", space_count, i, (strlen(envp[i]) + 1));
        space_count += (strlen(envp[i]) + 1);  // the length of each element in envp[]
        //Log("after end envp[%d] spce_count = %d", i, space_count);
    }
    for (int i = 0; i < argc; i++) { 
        //Log("argv spce_count: %d , argv[%d] len =  %d", space_count, i, (strlen(argv[i]) + 1));
        space_count += (strlen(argv[i]) + 1);  // the length of each element in argv[]
        //Log("after, end argv[%d] spce_count = %d", i, space_count);
    }
    Log("envpc: %d, argc: %d, space_count: %d", envpc, argc, space_count);


    Log("Base before ROUNDUP:%p", new_user_stack - space_count);
    uintptr_t *base = (uintptr_t *)ROUNDDOWN(new_user_stack - space_count, sizeof(uintptr_t));
    uintptr_t *base_mem = base;
    Log("Base after ROUNDUP:%p", base);

    
    // store argc, notice data type is int
    *((int *)base) = (int)argc;
    base = (uintptr_t *)((char *)base + sizeof(int));


    // first-level pointer to the address of a string(argv, envp)
    base += (argc + 1) + (envpc + 1);
    uintptr_t *string_base = (uintptr_t*)base;
    for (int i = 0; i < argc; i++) {
        memcpy((void *)base, (const void *)&argv[i], sizeof(void *));
        Log("copying argv %p <~ %p -> %s", base, argv[i], argv[i]);
    }

    for (int i = 0; i < envpc; i++) {
        memcpy((void *)base, (const void *)&envp[i], sizeof(void *));
        Log("copying envp %p <~ %p -> %s", base, envp[i], envp[i]);
    }


    // second-level pointer that stores the address 
    // where the string address is stored (the first-level pointer)
    base -= (argc + 1) + (envpc + 1);
    for (int i = 0; i < argc; i++, base++) {
        memcpy((void *)base, (const void *)&argv[i], sizeof(uintptr_t));
        Log("argv set: %p -> %p -> %s",
            base,
            *(char **)base,
            *(char **)base == NULL ? "(null)" : *(char **)base);
    }
    *base++ = (uintptr_t)NULL; // argv[argc] = NULL
    
    for (int i = 0; i < envpc; i++, base++) {
        memcpy((void *)base, (const void *)&envp[i], sizeof(uintptr_t));
        Log("envp set: %p -> %p -> %s",
            base,
            *(char **)base,
            *(char **)base == NULL ? "(null)" : *(char **)base);
    }
    *base++ = (uintptr_t)NULL; // argv[envpc] = NULL

    // Unspecified ...
    assert(string_base == base);

    pcb->cp->GPRx = (uintptr_t)base_mem;
}

