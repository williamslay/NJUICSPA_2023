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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static int buf_p =0; //buf pointer
static int over_flow =0;//over flow flag
static int call_times = 0;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";



static int choose(int max) { 
  int i = rand() % max;
  return  i;
}

static inline void array_look() {
  if(buf_p > 65535) {
    buf_p=0;
    over_flow = 1;
    memset(buf,0,sizeof(buf));
  }
}

static void gen_num() {
  if(buf_p >= 65536 - 33) {
    buf_p =0;
    over_flow = 1 ;
  }
  else {
    uint32_t num = choose(9999999)+1;
    buf_p+=sprintf(&buf[buf_p], "(unsigned int)%u", num);
  }
}

static void gen_rand_space() {
  switch (choose(2)) {
  case 1: 
  for(int i=0;i<=choose(4);i++) {
    array_look();
    buf[buf_p++] = ' '; 
  }
  break; 
  default:break;
  }
}

static void gen_rand_op() {
  array_look();
  switch (choose(4)) {
  case 1: buf[buf_p++] = '-';break; 
  case 2: buf[buf_p++] = '*';break;  
  case 3: buf[buf_p++] = '/';break;  
  default:buf[buf_p++] = '+';break;
  }
}

static void gen_rand_expr() {
  call_times ++;
  if(call_times >32)
   over_flow = 1;
  else{
    switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: array_look();buf[buf_p++]='(';gen_rand_space(); gen_rand_expr(); gen_rand_space();array_look();buf[buf_p++]=')'; break;
    default: gen_rand_expr();gen_rand_space(); gen_rand_op();gen_rand_space(); gen_rand_expr(); break;
  } 
  } 
}

static int end(){
  if(over_flow == 1)  {
    over_flow = 0;
    buf_p = 0;
    call_times = 0;
    memset(buf,0,sizeof(buf)); 
    return 1; 
  }
  else if(buf_p > 65535) {
    buf_p = 0;
    call_times = 0;
    memset(buf,0,sizeof(buf));
    return 1; 
  }
  else  {
    buf[buf_p++] = '\0';
    buf_p = 0;
    call_times =  0;
    return 0;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    do{
      gen_rand_expr();
    }
    while(end());
    
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Wall -Werror  /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);

    memset(buf,0,sizeof(buf)); 
  }
  return 0;
}


//awk '{gsub(/\(unsigned int\)/,""); print}' input  > input1
// we need this code to remove all the "(unsigned int)"