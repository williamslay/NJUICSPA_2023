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
#include <stdlib.h>
#include <stdio.h>


enum {
  TK_NOTYPE = 256, TK_EQ,TK_NUM,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},        // spaces
  {"\\+", '+'},             // plus
  {"\\-", '-'},             // sub
  {"\\*", '*'},             // times
  {"\\/", '/'},             // divided
  {"[0-9][0-9]*",TK_NUM},   // num
  {"\\(", '('},             // left bracket
  {"\\)", ')'},             // right bracket

  {"==", TK_EQ},            // equal
};

#define NR_REGEX ARRLEN(rules)

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
          case TK_NOTYPE: break; 
          case TK_NUM:
            tokens[nr_token].type = rules[i].token_type;
            Assert( substr_len <= 32,"Long Parameters.");
            memcpy(tokens[nr_token].str,substr_start,substr_len); 
            nr_token++;
            break; 
          default: tokens[nr_token++].type = rules[i].token_type ;
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

static bool check_brak(int p,int q) {
  int a = 0;
  for(int i=p;i<=q;i++) {
      switch (tokens[i].type) {
      case '(': a++;  break;
      case ')': if(a>0) a--; else return false; break; 
      default:break;
      }
  }
  if(a==0)  return true;
  else  return false; 
} 

static bool check_parentheses(int p,int q) {
  if(tokens[p].type!='(' || tokens[q].type!=')')  
    return false;
  else return check_brak(p+1,q-1); 
}


static int get_mainop(int p,int q) {
  int op = 0;
  int flag = 0;
  int pred=0;//predences
  for(int i=p;i<=q;i++) {
    switch (tokens[i].type) {
    case '(': flag++;  break;
    case ')': flag--;  break;
    case '+': case '-': if(flag == 0) {op = i; pred=1; break;} else break;
    case '*': case '/': if(flag == 0) {if(pred == 1) break; else op = i; break;} else break;
    default:break;
    }
  }
  return op;
}

static word_t eval(int p, int q) {
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    return atoi(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1);
  }
  else {
    int op = get_mainop(p,q);
    word_t val1 = eval(p, op - 1);
    word_t val2 = eval(op + 1, q);

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: assert(0);
    }
  }
}

static bool check_expr(int p ,int q)  {
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    if(tokens[p].type!=TK_NUM)
      return false;
    else return true;
  }
  else if (check_parentheses(p, q) == true) {
    return check_expr(p + 1, q - 1);
  }
  else  {
   int op = get_mainop(p,q);
    bool val1 = check_expr(p, op - 1);
    bool val2 = check_expr(op + 1, q);
    return val1&&val2;
  } 
}

word_t expr(char *e, bool *success) {
  memset(tokens,0,sizeof(tokens));
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  if(nr_token == 0 || !check_brak(0,nr_token-1) ||!check_expr(0,nr_token-1)) {
    printf("bad expression!\n");
    *success = false;
    return 0;
  }  
  
//  for (int i = 0; i < nr_token; i ++) {
//     if (tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == certain type) ) {
//       tokens[i].type = DEREF;
//     }
//   }

  return eval(0, nr_token-1);

}


void test_expr() {
    FILE *fp = fopen("/home/will/NJUPA/ics2023/nemu/tools/gen-expr/input", "r");
    assert(fp != NULL);

    char e[65536]={};
    word_t result,calculate,ret;
    int i =0;
    bool *success = NULL;

    do{
      i++;
      ret = fscanf(fp, "%d", &result);
      assert(ret != 0);
      assert(fgets(e,65536,fp)!=NULL);
      e[strlen(e)-1] = '\0'; 
      calculate=expr(e,success);
      Assert(calculate == result,"%d wrong result,result:%d,calculate:%d",i,result,calculate);
      Log("%d expr success",i);
      memset(e,0, sizeof(e));
    }while(!feof(fp));

    fclose(fp);
}