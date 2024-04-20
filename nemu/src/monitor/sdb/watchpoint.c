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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  char expr[128];
  word_t preval;
  int hittime;
  struct watchpoint *pre;
  struct watchpoint *next;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL ;
WP Dummyhead = {-1,"NULL",0,0,NULL,NULL};

void init_wp_pool() {
  for (int i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    memset(wp_pool[i].expr,0, sizeof(wp_pool[i].expr));
    wp_pool[i].preval = 0; 
    wp_pool[i].hittime = 0;
    wp_pool[i].pre = (i == 0 ? NULL : &wp_pool[i - 1]);
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }
  head = NULL;
  free_ = wp_pool;
  Dummyhead.next = head;
};

int new_wp(char* expr,bool * success) {
  if(free_ == NULL)  {
    *success = false;
    return 0;
  }
  else *success = true;
  WP* alloc = free_;
  free_ = free_->next;
  free_->pre = NULL;
  alloc->next = Dummyhead.next;
  alloc->pre = &Dummyhead; 
  if(head != NULL)head->pre = alloc;
  head = alloc;
  Dummyhead.next = head;
  strcpy(alloc->expr,expr);
  return alloc->NO;
};

void free_wp(int num , bool * success) {
  if(num <0 || num >= NR_WP) { 
    *success = false; 
    return;
  }
  WP*temp = head;
  while(temp != NULL) {
    if(temp->NO == num) {
      *success = true;
      /*delete from the head list*/
      temp->pre->next = temp->next;
      if(temp->next != NULL)temp->next->pre = temp->pre;
      /*reset and push back to the free_ list*/
      memset(temp->expr,0, sizeof(temp->expr));
      temp->hittime = 0;
      temp->preval = 0;
      temp->pre = NULL;
      temp->next = free_;
      if(free_ != NULL)free_->pre = temp;
      free_ = temp; 
      head = Dummyhead.next;
      return;
    }
    temp = temp->next;
  }
  *success = false;
};

void wp_display()
{
  printf("Num \t What \t \n");
  WP*temp = Dummyhead.next; 
  while(temp != NULL){
    printf("%d\t %s\t \n",temp->NO,temp->expr);
    if(temp->hittime > 0) printf(" breakpoint already hit %d times\n",temp->hittime);
    temp = temp->next;
  }
}

bool wp_test() {
  bool flag = false; 
  WP*temp = Dummyhead.next; 
  while(temp != NULL) {
    word_t newval = expr(temp->expr,NULL);
    if(newval != temp->preval) {
      flag = true;
      printf("Watchpoint %d : %s\n",temp->NO,temp->expr);
      temp->hittime++;
      printf("Old Value = %d\n",temp->preval);
      temp->preval = newval;
      printf("New Value = %d\n",temp->preval); 
      printf("\n");
    }
    temp = temp->next;
  }
  return flag;
}