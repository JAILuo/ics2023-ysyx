#include <memory.h>
#include <fs.h>
#include <ramdisk.h>
#include <proc.h>
#include <elf.h>
#include <stddef.h>
#include <stdint.h>


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

#define ROUNDUP(a, sz)      ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))
#define ROUNDDOWN(a, sz)    ((((uintptr_t)a)) & ~((sz) - 1))


uintptr_t calc_aligned_page(uintptr_t start, size_t size, size_t *nr_page) {
    const uintptr_t start_aligned = (start) & ~((uintptr_t)(PGSIZE) - 1); //down
    const uintptr_t end_aligned = (((uintptr_t)start + size) + (PGSIZE) - 1) & ~((PGSIZE) - 1);
    printf("start_aligned: 0x%x  end_aligned: 0x%x  nr_page: 0x%x\n",
           start_aligned, end_aligned, (end_aligned - start_aligned)/PGSIZE);
    *nr_page = (end_aligned - start_aligned) / PGSIZE;
    return start_aligned;
}

//static int cptest = 1;
static uintptr_t loader(PCB *pcb, const char *filename) {
    int fd = fs_open(filename, 0, 0);

    //Log("filename:%s %p",filename, filename);

    Elf_Ehdr eh;
    fs_read(fd, &eh, sizeof(Elf_Ehdr));

    // check magic number
    for (int i = 0; i < 16; i++) assert(magic[i] == eh.e_ident[i]);

    // check machine  
    assert(eh.e_machine == EXPECT_TYPE);

    Elf_Phdr ph[eh.e_phnum]; 
    fs_lseek(fd, eh.e_phoff, SEEK_SET);
    fs_read(fd, ph, sizeof(Elf_Phdr) * eh.e_phnum);
    for (size_t i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {

            //fs_lseek(fd, ph[i].p_offset, SEEK_SET);
            //fs_read(fd, (void *)ph[i].p_vaddr, ph[i].p_memsz);// vaddr or paddr？
            //memset((void *)(uintptr_t)ph[i].p_vaddr + ph[i].p_filesz, 0, 
            //       ph[i].p_memsz - ph[i].p_filesz);
            
            printf("ph.p_vaddr: 0x%x  ph.p_memsz: 0x%x  ph.p_filesz: 0x%x\n",
                   ph[i].p_vaddr, ph[i].p_memsz, ph[i].p_filesz);
            size_t nr_page = 0;
            const uintptr_t start_addr = calc_aligned_page(ph[i].p_vaddr, ph[i].p_memsz, &nr_page);
            void *p_page = new_page(nr_page);
            void *const pages_start = p_page + (ph[i].p_vaddr - start_addr);
            assert(nr_page * PGSIZE >= ph[i].p_memsz);

            fs_lseek(fd, ph[i].p_offset, SEEK_SET);
            fs_read(fd, pages_start, ph[i].p_filesz);
            memset(pages_start + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);

            printf("start_addr: 0x%x  p_page: %p  pages_start: %p\n",
                   start_addr, p_page, pages_start);
            for (size_t j = 0; j < nr_page; j++) {
                map(&pcb->as,
                    (void *)start_addr + PGSIZE * j,
                    p_page + PGSIZE * j,
                    PTE_V | PTE_R | PTE_W | PTE_X);
            }

        }
    }
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

static size_t len_varargs(char *const varargs[]) {
  if (varargs == NULL) return 0;

  Log("scanning varargs list [%p]", varargs);
  size_t len = 0;
  while (varargs[len] != NULL) {
    Log("+0x%x: %p -> %p -> %s", sizeof(char *) * len, &varargs[len], varargs[len], varargs[len]);
    len++;
  }
  Log("+0x%x: %p -> %p", sizeof(char *) * len, &varargs[len], varargs[len]);
  return len;
}

//static int test = 1;
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
    // craete address space
    protect(&pcb->as);
    // create new user stack for new user process
    void *new_user_stack_top = new_page(8);
    void *new_user_stack_bottom = new_user_stack_top + 8 * PGSIZE;
    Log("new_user_stack_bottom: %p, new_user_stack_top: %p",
        new_user_stack_bottom, new_user_stack_top);


    // store argc, argv, envp in the user stack after ucontext.
    const int argc = (int)len_varargs(argv);
    const int envpc = (int)len_varargs(envp);

    size_t space_count = 0;
    space_count += sizeof(int); // store argc
    //Log("argc spce_count: %d", space_count);
    space_count += sizeof(uintptr_t) * (argc + 1); // store argv
    //Log("argv: %d spce_count: %d", argc, space_count);
    space_count += sizeof(uintptr_t) * (envpc + 1); // store envp
    //Log("envpc: %d spce_count: %d", envpc, space_count);
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


    space_count += sizeof(uintptr_t); // for ROUNDUP
    Log("Base before ROUNDUP:%p", new_user_stack_bottom - space_count);
    uintptr_t *base = (uintptr_t *)ROUNDUP(new_user_stack_bottom - space_count, sizeof(uintptr_t));
    uintptr_t *base_2_app = base;
    Log("Base after ROUNDUP:%p", base);
    
    // store argc, notice data type is int
    *((int *)base) = (int)argc;
    Log("argc set: %p -> 0x%x", base, *(int *)base);
    base = (uintptr_t *)((char *)base + sizeof(int));


    // second-level pointer that stores the address 
    // where the string address is stored (the first-level pointer)
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


    // stirng area
    // first-level pointer to the address of a string(argv, envp)
    for (int i = envpc - 1; i >= 0; i--) {
        memcpy((void *)base, (const void *)&envp[i], sizeof(void *));
        base = (uintptr_t *)((char *)base + (strlen(envp[i]) + 1));
        Log("copying envp(base) %p <~ %p -> %s", base, envp[i], envp[i]);
    }
    for (int i = argc - 1; i >= 0; i--) {
        memcpy((void *)base, (const void *)&argv[i], sizeof(void *));
        base = (uintptr_t *)((char *)base + (strlen(argv[i]) + 1));
        Log("copying argv(base) %p <~ %p -> %s", base, argv[i], argv[i]);
    }


    Area kstack = {
        .start = pcb->stack,
        .end   = pcb->stack + STACK_SIZE
    };
    Log("kernel stack.start/stack_top: %p, "
        "stack.end/stack_bottom: %p",
        kstack.start, kstack.end);

    // create new context for new process
    uintptr_t entry = loader(pcb, filename);
    Log("switch to user process: %s  entry:", filename, entry);
    pcb->cp = ucontext(&pcb->as, kstack, (void(*)())entry);

    pcb->cp->GPRx = (uintptr_t)base_2_app;

    void *ustack_top_vaddr = pcb->as.area.end - 8 * PGSIZE;
    for (int i = 0; i < 8; i++) {
        map(&pcb->as,
            ustack_top_vaddr + (PGSIZE * i),
            new_user_stack_top + (PGSIZE * i), 0);
    }
    /*
    Log("user context: %p (a0=%p, sp=%p)", 
        pcb->cp,
        (void *)pcb->cp->GPRx,
        (void *)pcb->cp->gpr[2]);
        */
}
