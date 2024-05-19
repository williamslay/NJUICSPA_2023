/***************************************************************************************
* Copyright (c) 2024 ,Shangyuan Lei
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

#include <common.h>
#include <macro.h>
#include <utils.h>
#include <elf.h>

#ifdef CONFIG_FTRACE

#define INST_LENGTH 4 //for riscv-32/64,mips32
#define lOG_LENGTH 256
#define FUCNAME_LENGTH 128
#define FTRACE_MAX 65536

typedef struct func_node {
  vaddr_t begin;
  vaddr_t end;
  char name[FUCNAME_LENGTH]; 
  struct func_node *next;
} FM_NODE;

typedef struct ftrace_node {
  char log_buff[lOG_LENGTH]; 
  struct ftrace_node *next;
} FT_NODE;

typedef MUXDEF(CONFIG_ISA64,  Elf64_Ehdr,  Elf32_Ehdr) Elf_Ehdr;
typedef MUXDEF(CONFIG_ISA64,  Elf64_Off,   Elf32_Off)  Elf_Off;
typedef MUXDEF(CONFIG_ISA64,  Elf64_Shdr,  Elf32_Shdr) Elf_Shdr;
typedef MUXDEF(CONFIG_ISA64,  Elf64_Sym,   Elf32_Sym)  Elf_Sym;

typedef size_t (*fread_func_ptr_t)(void *, size_t, size_t, FILE *);

__attribute__((unused))
size_t wrapped_fread(void *ptr, size_t size, size_t count, FILE *stream) {
    fread_func_ptr_t fread_ptr = (fread_func_ptr_t)fread;
    return fread_ptr(ptr, size, count, stream);
}

static FT_NODE * que_head = NULL , *que_tail = NULL;
static FM_NODE * map_head = NULL ;
static int st_count = 0 , que_count = 0;

void add_map_node (char * func_name,vaddr_t func_begin , vaddr_t func_end) {
  struct func_node *ptr = (struct func_node *)malloc(sizeof(struct func_node));
  ptr->begin = func_begin;
  ptr->end = func_end;
  memset(ptr->name, '\0', sizeof(ptr->name));
  strcpy(ptr->name , func_name);
  ptr->next = map_head;
  map_head = ptr;
}

char* get_func_name (vaddr_t addr) {
  FM_NODE * temp = map_head;
  while(temp != NULL) {
    if(addr >= temp->begin && addr <= temp->end) return temp->name;
    temp = temp->next;
  }
  char *wrong_ans = "???";
  return wrong_ans; 
}

Elf_Shdr get_section_header(FILE *fp, Elf_Ehdr elf_header, int index) {
  Elf_Shdr section_header;
  fseek(fp, elf_header.e_shoff + index * sizeof(Elf_Shdr), SEEK_SET);
  wrapped_fread(&section_header, sizeof(Elf_Shdr), 1, fp);
  return section_header;
}

Elf_Shdr find_section_by_name(FILE *fp, Elf_Ehdr elf_header, const char *section_name) {
  Elf_Shdr section_header;
  for (int i = 0; i < elf_header.e_shnum; ++i) {
    section_header = get_section_header(fp, elf_header, i);
    fseek(fp, elf_header.e_shoff + elf_header.e_shstrndx * sizeof(Elf_Shdr), SEEK_SET);
    Elf_Shdr shstrtab_header;
    wrapped_fread(&shstrtab_header, sizeof(Elf_Shdr), 1, fp);
    char name[shstrtab_header.sh_size];
    fseek(fp, shstrtab_header.sh_offset + section_header.sh_name, SEEK_SET);
    wrapped_fread(name, shstrtab_header.sh_size, 1, fp);
    if (strcmp(name, section_name) == 0) {
      return section_header;
    }
  }
  // Return an empty section header if the section is not found
  return (Elf_Shdr){0};
}

char* get_symbol_name(FILE *fp, Elf32_Shdr strtab_section,  uint32_t st_name_offset) {
  // Allocate memory for the symbol name
  char *name = (char *)malloc(FUCNAME_LENGTH);
  if (name == NULL) {
    perror("Memory allocation failed");
    return NULL;
  }

  // Seek to the beginning of the string table section
  fseek(fp, strtab_section.sh_offset, SEEK_SET);

  // Seek to the position of the symbol's string
  fseek(fp, st_name_offset, SEEK_CUR);

  // Read the symbol's string
  int i = 0;
  char c;
  while (fread(&c, sizeof(char), 1, fp) == 1 && c != '\0' && i < 255) {
    name[i++] = c;
  }
  name[i] = '\0'; // Null-terminate the string
  return name;
}

void init_func_que() {
  que_head = (struct ftrace_node *)malloc(sizeof(struct ftrace_node));
  memset(que_head->log_buff,'\0',sizeof(que_head->log_buff));
  que_tail = que_head;
}

int init_func_map(const char *elf_file) {
  if(elf_file == NULL) return 0;

  FILE *fp = fopen(elf_file, "rb");
  Elf_Ehdr elf_header;
  Elf_Sym symbol;

  // read elf header
  wrapped_fread(&elf_header, 1, sizeof(Elf_Ehdr), fp);

  // find the ".symtab" section and ".strtab" section 
  Elf_Shdr symtab_header = find_section_by_name(fp, elf_header, ".symtab");
  Elf_Shdr strtab_header = find_section_by_name(fp, elf_header, ".strtab");

  int symbol_count = symtab_header.sh_size / symtab_header.sh_entsize;

  fseek(fp, symtab_header.sh_offset, SEEK_SET);

  // read the symbol table
  for (int j = 0; j < symbol_count; j++) {
    wrapped_fread(&symbol, 1, sizeof(Elf_Sym), fp);
    if(symbol.st_size == 0) continue; //exclude the "_start" function
    if(MUXDEF(CONFIG_ISA64, ELF64_ST_TYPE(symbol.st_info),ELF32_ST_TYPE(symbol.st_info)) ==  STT_FUNC) {
      long file_pos = ftell(fp);
      char* symbolName = get_symbol_name(fp, strtab_header, symbol.st_name);
      add_map_node(symbolName,(vaddr_t)symbol.st_value,(vaddr_t)(symbol.st_value + symbol.st_size - INST_LENGTH)); 
      free(symbolName);
      fseek(fp, file_pos, SEEK_SET);
    }
  } 
  
  fclose(fp);
  init_func_que();
  return 1;
}

void add_func_que(int type, vaddr_t now_addr,vaddr_t next_addr) {
  Assert(que_count <= FTRACE_MAX,"trace too many functions,there may be someting wrong!");
  char* now_func_name = get_func_name(now_addr);
  char* next_func_name = get_func_name(next_addr); 
  if(strcmp(now_func_name,next_func_name)== 0) return; 
  struct ftrace_node* ptr = (struct ftrace_node *)malloc(sizeof(struct ftrace_node));
  ptr->next = NULL;
  char* pos = ptr->log_buff;
  pos+= sprintf(pos,FMT_WORD":",now_addr);
  /*type 0 : call ,type 1: ret */
  if(type == 0) {
    st_count++;
    pos+= sprintf(pos,"%*s", st_count, "");
    pos+= sprintf(pos,"call [%s@"FMT_WORD"]",next_func_name,next_addr); 
  }else{
    pos+= sprintf(pos,"%*s", st_count, "");
    pos+= sprintf(pos,"ret [%s]",now_func_name); 
    st_count--;
  }
  que_tail->next = ptr;
  que_tail = que_tail->next;
  que_count++;
}

void p_ftrace() {
  FT_NODE * temp = que_head->next;
  if(temp != NULL) printf(ANSI_FMT("FuncTrace will show functions traced before program crushes/stops below.\n",ANSI_FG_GREEN ));
  while(temp != NULL) {
    printf("%s\n",temp->log_buff);
    temp = temp->next;
  }
}

#endif