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
static char expr_buf[65536] = {};
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
int mul = 0;

int choose(int n){
  return rand()%n;
}

void gen_rand_space(){
  char space[2] = {' ', '\0'};
  switch(choose(2)){
    case 0:break;
    default:strcat(buf, space);break;
  }
}

void gen_num(){
  char num_buffer[1024];
  num_buffer[0] = '\0';
  uint32_t number = rand() % 100 + 1;
  sprintf(num_buffer ,"%d", number);
  strcat(buf, num_buffer);
}

void gen(char c){
  char cha_buffer[2] = {c, '\0'};
  strcat(buf, cha_buffer);
}

void gen_rand_op(){
  char op[2] = {'a', '\0'};
  switch(choose(4)){
    case 0:op[0] = '+';mul = 0;break;
    case 1:op[0] = '-';mul = 0;break;
    case 2:
      if(mul<2){
        op[0] = '*';
        mul += 1;
        break;
      }
    default:op[0] = '/';mul = 0;break;
  }
  strcat(buf, op);
}

int gen_result(char *str){
    sprintf(code_buf, code_format, str);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) ;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    return result;
}

static void gen_rand_expr(int depth) {
  int p = strlen(buf);
  if(depth == 1 || depth == 2){
    gen_num();
    depth -= 1;
  }
  else{
    switch(choose(2)){
      case 0:depth -= 2; gen('('); gen_rand_space(); gen_rand_expr(depth); gen_rand_space(); gen(')');break;
      default:depth -= 1;int i = choose(depth - 1) + 1; gen_rand_expr(i);gen_rand_space(); gen_rand_op(); gen_rand_space(); gen_rand_expr(depth-i);break;
    }
  }
  int q = sizeof(buf);
  memcpy(expr_buf, buf+p, q-p);
  int result = gen_result(expr_buf);
  if(result == 0 && p !=0 && (buf[p-1] == '/' || (buf[p-1] ==' ' && buf[p-2] == '/'))){
    buf[p] = '\0';
    gen_num();
  }
  expr_buf[0] = '\0';
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
    int depth = choose(32)+1;
    gen_rand_expr(depth);

    int result;
    result = gen_result(buf);

    printf("%u %s\n", result, buf);
    buf[0] = '\0';
  }
  return 0;
}
