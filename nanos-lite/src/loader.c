#include <proc.h>
#include <elf.h>

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
# define EXPECT_TYPE EM_MIPS
#elif defined(__ISA_LOONGARCH32R__)
# define EXPECT_TYPE EM_LOONGARCH
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#endif

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
extern size_t get_ramdisk_size();

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr elf_header;
  Elf_Phdr pHdr;
  // read elf header
  ramdisk_read(&elf_header, 0, sizeof(Elf_Ehdr));
  // check the magic number and ISA
  assert(elf_header.e_ident[0] == 0x7F && elf_header.e_ident[1] == 'E'
      && elf_header.e_ident[2] == 'L' && elf_header.e_ident[3] == 'F');
#ifndef EXPECT_TYPE
# define EXPECT_TYPE  EM_NONE
#endif
  assert(elf_header.e_machine == EXPECT_TYPE);
  // start loading
  for (int i = 0; i < elf_header.e_phnum; i++) {
    ramdisk_read(&pHdr, elf_header.e_phoff + i * sizeof(Elf_Phdr), sizeof(Elf_Phdr));
    if (pHdr.p_type != PT_LOAD) continue;
    ramdisk_read((uintptr_t *) pHdr.p_vaddr, pHdr.p_offset, pHdr.p_memsz);
    memset((uintptr_t *) (pHdr.p_vaddr + pHdr.p_filesz), 0, pHdr.p_memsz - pHdr.p_filesz);
  }
  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}