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

#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <limits.h>
#include <utils.h>

static int is_batch_mode = false;
extern int ftrace;

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}


static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  int arg;
  if (args == NULL )
    arg = 1;
  else{
    arg = atoi(args);
    if(arg < 1 || arg >INT_MAX)  {
      printf("The argument given is in wrong range !\n");
      return 0;
    }
  }
  cpu_exec(arg);
  return 0;
}

static int cmd_info(char *args) {
  if(args == NULL || strlen(args) != 1) {
    printf("Need or illegal parameters!\n");
    return 0;
  }
  switch (args[0]){
    case 'r': isa_reg_display(); break;
    case 'w': wp_display(); break;
  }
  return 0;
}

static int cmd_x(char *args) {
  if(args == NULL) {
    printf("Need parameters!\n");
    return 0;
  } 
  char *str_end = args + strlen(args);
 /* extract the first argument */
  char *num = strtok(NULL, " ");
  int n = atoi(num);
  char *addr_exp = num + strlen(num) + 1;
  if (addr_exp >= str_end) {
    addr_exp = NULL;
    printf("Need right parameters.\n");
    return 0;
  } 
  vaddr_t addr = strtoul(addr_exp,NULL,16);
  for(int i=0;i<n;i++) {
    word_t val = vaddr_read(addr+i*4,4);
    if( i%4 == 0 ) {
      printf(ANSI_FMT(FMT_WORD": ", ANSI_FG_BLUE) ,addr+i*4);
    }
    printf(FMT_WORD"\t",val);
    if( i%4 == 3 ) {
      printf("\n"); 
    }
  }
  printf("\n");  
  return 0;
}

static int cmd_p(char *args) {
  if(args != NULL) {
    bool success = true;
    word_t val = expr(args,&success);
    if(success) printf("$ = "FMT_WORD" (%d)\n",val,val);
  }else printf("Need parameters!\n");
  return 0;
}

static int cmd_w(char *args) {
  if(args != NULL) {
    bool success = true; 
    int wid = new_wp(args,&success);
    if(success) printf("Watchpoint %d : %s\n",wid,args);
    else printf("Could not insert watchpoint!\n"); 
  }else printf("Need parameters!\n"); 
  return 0;
}

static int cmd_d(char *args) {
  if(args != NULL) {
    bool success = true;
    int num = atoi(args);
    free_wp(num,&success);
    if(success) printf("Watchpoint %d has been sucessfuly deleted!\n",num);
  }else printf("Need parameters!\n"); 
  return 0;
}

static int cmd_ft(char *args) {
#ifdef CONFIG_FTRACE
  if(ftrace) p_ftrace();
  else printf("There is something wrong with your elf file!\n"); 
#else
  printf("This function has not been set yet!\n");
#endif 
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","Let the program pause after executing N instructions in a single step.When N is not given, the default is 1",cmd_si},
  { "info","Print the state of the program. The subcommands are 'r' for register and 'w' for watchpoint infomation",cmd_info},
  { "x","Find the value of the expression EXPR, use the result as the starting memory address, and output N consecutive 4-byte outputs in hexadecimal",cmd_x},
  { "p", "Calculate the value of the expression EXPR", cmd_p },
  { "w", "Set watchpoint to stop execution whenever the value of the given expression changes", cmd_w },
  { "d", "Delete a watchpoint based on the given watchpoint number", cmd_d },
  { "ftrace", "Show the function traces if you have set this function", cmd_ft }  
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  /* Initialize the itrace ring buffer. */
  init_iringbuf();
}
