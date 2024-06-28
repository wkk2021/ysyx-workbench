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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>


static int is_batch_mode = false;


void init_regex();
void init_wp_pool();

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
  return -1;
}

static int cmd_si(char *args){
  char *arg = strtok(args, " ");
  int n;
  if(arg == NULL){
      n = 1;
  }
  else{
      n=atoi(args);
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args){
  char *arg = strtok(args, " ");
  if(*arg == 'r'){
    isa_reg_display();
  }
  else if(*arg == 'w'){
    WP *p = head;
    while(p != NULL){
      if(p->hex == 0){
        printf("The No.%d watchpoint, its EXPR is \"%s\", and the value is %d\n", p->NO, p->expr, p->value);
        p = p->next;
      }
      else if(p->hex == 1){
        printf("The No.%d watchpoint, its EXPR is \"%s\", and the value is 0x%08X\n", p->NO, p->expr, p->value);
        p = p->next;
      }
    }
  }
     return 0;
}

static int cmd_x(char *args){
  int n = atoi(args);
  char *arg = strtok(args, " ");
  arg = strtok(NULL, " ");
  char *str;
  int addr = strtol(arg, &str, 16);
  int i;
  for(i=0;i<n;i++){
    int a = paddr_read(addr + i*4, 4);
    printf("0x%X = 0x%08X\n", addr + 4*i, a);
  }
  return 0;
}

static int cmd_p(char *args){
  bool success;
  int v = expr(args, &success);
  if (success){
    if(hex_sign == 0){
      printf("%s = %u\n", args, v);
    }
    else if(hex_sign == 1){
      printf("%s = 0x%08X\n", args, v);
    }
  }
  return 0;
}


/*static int cmd_p(char *args){
  FILE *fp;
  char str1[256];
  char *lines[10000];
  int num_lines = 0;
  fp = fopen("input", "r");
  if(fp == NULL) {
      perror("打开文件时发生错误");
  }
  else{
    while(fgets (str1, 256, fp)!=NULL){
      lines[num_lines] = malloc(strlen(str1)+1);
      strcpy(lines[num_lines], str1);
      num_lines++;
    }
  }
  fclose(fp);
  
  
  for(int i = 0; i<10000; i++){
    fp = fopen("input_com.txt", "a");
    int result = atoi(lines[i]);
    char *str2 = malloc(256);
    for(int j = 0; j < strlen(lines[i]); j++){
      if(*(lines[i]+j) == ' '){
        strcpy(str2, lines[i] + j + 1);
        //str2 = lines[i] + j + 1;
        break;
      }
    }
    str2[strlen(str2)-1] = '\0';
    bool success;
    uint32_t v = expr(str2, &success);
    char s[3] = {' ', 'a', '\0'};
    s[1] = (result == v)? '1':'0';
    strcat(str2, s);
    fprintf(fp, "%u %u %s\n",result, v, str2);
    fclose(fp);
    free(lines[i]);
    free(str2);
  }
  
  return 0;
  
}
*/


static int cmd_w(char *args){
  WP *p;
  bool success;
  int value = expr(args, &success);
  p = new_wp(args, value, hex_sign);
  wp(p);
  return 0;
}

static int cmd_d(char *args){
  WP *p1;
  WP *p2;
  p1 = head;
  p2 = NULL;
  int NO = atoi(args);
  while(p1 != NULL){
    if(p1->NO == NO){
      free_wp(p2);
      break;
    }
    else{
      p2 = p1;
      p1 = p1->next;
    }
  }
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
  { "si", "\"si [N]\":Let the program step into N instructions and then pause the execution, when N is not given, the default is 1", cmd_si },
  {"info", "\"info r:Print register status; info w:Print the monitoring point information", cmd_info},
  {"x", "Find the value of the expression EXPR, take the result as the starting memory address, and output N consecutive 4 bytes in hexadecimal form", cmd_x},
  {"p", "p EXPR: it can give the result of EXPR", cmd_p},
  {"w", "w EXPR: it can set a watch point", cmd_w},
  {"d", "d NO: it can delete a watch point with number of NO", cmd_d}

  /* TODO: Add more commands */

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
}
