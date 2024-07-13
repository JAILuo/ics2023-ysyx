#include <proc.h>
#include <elf.h>

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


static uintptr_t loader(PCB *pcb, const char *filename) {
    Elf_Ehdr eh;
    ramdisk_read(&eh, 0, sizeof(Elf_Ehdr));
    assert(*(uint32_t *)eh.e_ident == 0x464c457f); //little endian 7f 45 4c 46

    /* check machine  */ 
    assert(eh.e_machine == EXPECT_TYPE);

    Elf_Phdr ph[eh.e_phnum];
    ramdisk_read(ph, eh.e_phoff, sizeof(Elf_Phdr) * eh.e_phnum);
    for (int i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type == PT_LOAD) {
            ramdisk_read((void *)ph[i].p_paddr, ph[i].p_offset, ph[i].p_memsz);
            memset((void *)ph[i].p_paddr + ph[i].p_filesz, 0, 
                   ph[i].p_memsz - ph[i].p_filesz);
        }
    }
    return eh.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

