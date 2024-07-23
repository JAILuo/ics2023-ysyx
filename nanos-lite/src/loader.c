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

void context_uload(PCB *pcb, const char *process_name) {
    uintptr_t entry = loader(pcb, process_name);
    Area stack = {
        .start = heap.end - STACK_SIZE,
        .end   = heap.end
    };
    Log("name: %s", process_name);
    Log("entry: %d", entry);
    Log("stack.start: %d, stack.end: %d", stack.start, stack.end);

    pcb->cp = ucontext(&pcb->as, stack, (void *)entry);
    pcb->cp->GPRx = (uintptr_t)heap.end;
}


