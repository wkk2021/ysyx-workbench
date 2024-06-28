/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2->
* You can use this software according to the terms and conditions of the Mulan PSL v2->
* You may obtain a copy of Mulan PSL v2 at:
*          http://license->coscl->org->cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE->
*
* See the Mulan PSL v2 for more details->
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32

WP* new_wp();
void free_wp(WP *wp);
int nr_wp = 0;

/*typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  TODO: Add more members if necessary 
  char expr[256];
  int value;
  
} WP;
*/
static WP wp_pool[NR_WP] = {};
WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].expr[0] = '\0';
    wp_pool[i].value = 0;
    wp_pool[i].hex = 0;
  }

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp(char *args, int value, int hex){
  nr_wp++;
  WP *p = NULL;
  if(free_ == NULL){
    assert(0);
  }
  else{
    p = free_;
    free_ = free_->next;
    p->next = NULL;
  }
  p->NO = nr_wp;
  p->value = value;
  p->hex = hex;
  strcpy(p->expr, args);
  return p;
}

void wp(WP *p){
  p->next = head;
  head = p;
}

void free_wp(WP *wp){
  if(wp == NULL){
    WP *p = head->next;
    head->next = free_;
    free_ = head;
    head = p;
  }
  else{
    WP *p = wp->next;
    wp->next = p->next;
    p->next = free_;
    free_ = p;
  }
}

/* TODO: Implement the functionality of watchpoint */

