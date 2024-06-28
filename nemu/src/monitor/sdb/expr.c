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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>

enum {
  TK_NOTYPE = 1, TK_EQ,
  TK_NUM, TK_UEQ, TK_NBIG, TK_NSMALL,
  MINUS, REG, HEX, DEREF

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {"\\$[$a-z0-9]{2}", REG},
  {"0x[0-9a-zA-Z]{8}+", HEX},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},
  {"\\*", '*'},
  {"\\/", '/'},
  {"[0-9]+", TK_NUM},
  {"!=", TK_UEQ},
  {"<=", TK_NBIG},
  {">=", TK_NSMALL},
  {"<", '<'},
  {">", '>'},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\|\\|", '|'},
  {"&&", '&'}
};

#define NR_REGEX ARRLEN(rules)
int hex_sign = 0;
static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {
          case REG:hex_sign = 1;
          case HEX:hex_sign = 1;
          case TK_NUM:
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          case TK_NOTYPE:break;
          case '-':
            if(nr_token == 0 || (nr_token > 0 && tokens[nr_token-1].type != TK_NUM && tokens[nr_token-1].type != ')' && tokens[nr_token-1].type != REG && tokens[nr_token-1].type != HEX)){
              tokens[nr_token].type = MINUS;
              nr_token++;
              break;
            }
          case '*':
            if(nr_token == 0 || (nr_token > 0 && tokens[nr_token-1].type != TK_NUM && tokens[nr_token-1].type != ')' && tokens[nr_token-1].type != REG && tokens[nr_token-1].type != HEX)){
              tokens[nr_token].type = DEREF;
              nr_token++;
              break;
            }
          case '/':
          case '+':
          case TK_EQ:
          case TK_UEQ:
          case TK_NBIG:
          case TK_NSMALL:
          case '>':
          case '<':
          case '(':
          case ')':
          case '|':
          case '&':
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          default: TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int prio(char type){
  switch(type){
   case '|':return 1;break;
   case '&':return 2;break;
   case TK_EQ:
   case TK_UEQ:return 3;break;
   case TK_NBIG:
   case TK_NSMALL:
   case '>':
   case '<':return 4;break;
   case '+':return 5;break;
   case '-':return 6;break;
   case '*':return 7;break;
   case '/':return 8;break;
   default:assert(0);
  }
}

bool check_parentheses(int p, int q){
  int num = 0;
  if(tokens[p].type == '(' && tokens[q].type == ')' && (q - p) >= 2){
    for(int i = p+1; i<q; ++i){
      if(tokens[i].type == '('){
        num += 1;
      }
      else if(tokens[i].type == ')'){
        num -= 1;
      }
    }
    if(num == 0){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }    
}

int major_token(int p, int q){
  int d = 3;
  int d_position = 0;
  int mt = 10;
  int j = 0;
  int position = 0;
  int k = 0;
  for(int i = p; i <=q; i++){
    if(tokens[i].type == '('){
      j -= 1;
    }
    if(tokens[i].type == ')'){
      j += 1;
    }
    if(tokens[i].type != TK_NUM && j == 0 && tokens[i].type != ')' && tokens[i].type != MINUS && tokens[i].type != DEREF && tokens[i].type != HEX && tokens[i].type != REG){
      k = prio(tokens[i].type);
      if(k == 7 && d !=0){
        d = 1;
      }
      else if(k == 8 && d != 0){
        d = 2;
        d_position = i;
      }
      else{
        d = 0;
      }
      if(mt >= k){
        mt = k;
        position = i;
      }
    }
  }
  if(d == 2){
    position = d_position;
  }
  return position;
}

int eval(int p, int q){
  int ans = 0;
  if(p > q){
    assert(0);
  }
  else if(p == q){
    if(tokens[p].type == TK_NUM){
      ans = atoi(tokens[p].str);
    }
    else if(tokens[p].type == REG){
      bool success;
      int a = isa_reg_str2val(tokens[p].str, &success);
      if(success){
        ans = a;
      }
      else{
      assert(0);
      }
    }
    else if(tokens[p].type == HEX){
      char *str;
      ans = strtol(tokens[p].str, &str, 16);
    }
  }
  else if(q > p ){
    int mt = major_token(p, q);
    if(mt == 0){
      if(tokens[p].type==MINUS){
        ans = -eval(p+1, q);
      }
      else if(tokens[p].type=='('){
        ans = eval(p+1, q-1);
      }
      else if(tokens[p].type==DEREF){
        int addr = eval(p+1, q);
        int a = paddr_read(addr, 4);
        ans = a;
      }
    }
    else{
      int op1 = eval(p, mt-1);
      int op2 = eval(mt+1, q);
      switch(tokens[mt].type){
        case '+':ans = op1 + op2; break;
        case '-':ans = op1 - op2; break;
        case '*':ans = op1 * op2; break;
        case '/':ans = op1 / op2; break;
        case TK_EQ:ans = op1 == op2; break;
        case TK_UEQ:ans = op1 != op2; break;
        case TK_NBIG:ans = op1 <= op2; break;
        case TK_NSMALL:ans = op1 >= op2; break;
        case '<':ans = op1 < op2; break;
        case '>':ans = op1 > op2; break;
        case '|':ans = op1 || op2; break;
        case '&':ans = op1 && op2; break;
        default:assert(0);   
      } 
    }
  }
  return ans;
}

word_t expr(char *e, bool *success) {
  hex_sign = 0;
  int ans = 0;
  if (!make_token(e)) {
    *success = false;
  }
  else{
    ans = eval(0, nr_token-1);
    *success = true;
  }
  return ans;
  
  /* TODO: Insert codes to evaluate the expression. */
 // TODO();
}
