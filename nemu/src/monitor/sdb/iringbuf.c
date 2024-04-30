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
#include "sdb.h"

#define BUFF_SIZE 20
#define lOG_LENGTH 128

typedef struct itrace_node {
  char log_buff[lOG_LENGTH];  
  struct itrace_node *next;
} IT_NODE;

static IT_NODE iringbuffer[BUFF_SIZE] = {};
static IT_NODE * ptr = NULL;
static int node_num = 0;

void init_iringbuf() {
  for (int i = 0; i < BUFF_SIZE; i ++) {
    memset(iringbuffer[i].log_buff,0, sizeof(iringbuffer[i].log_buff)); 
    iringbuffer[i].next = (i == BUFF_SIZE - 1 ? &iringbuffer[0] : &iringbuffer[i + 1]);
  }
  ptr = iringbuffer;
};

void itra_write(char* itrace) {
    strcpy(ptr->log_buff,itrace);
    if(node_num < BUFF_SIZE) node_num++; 
    ptr = ptr->next;
};

void itra_log() {
    printf(ANSI_FG_GREEN "ITraceRing will show %d instruction(s) traced before program crushes below." ANSI_NONE "\n",node_num);
    IT_NODE * temp = ptr;
    int k = BUFF_SIZE - node_num;
    while(k-- > 0) {
        temp = temp->next;
    }
    while(temp->next != ptr) {
        printf("%s\n",temp->log_buff);
        temp = temp->next;
    }
    printf(ANSI_FG_RED "%s" ANSI_NONE "\n",temp->log_buff);
    printf("\n");
};