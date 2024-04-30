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
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>


enum {
  TK_NOTYPE = 256, TK_EQ,TK_DNUM,TK_NEQ,TK_AND,TK_HNUM,TK_REG,DEREF
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},                // spaces
  {"\\+", '+'},                     // plus
  {"\\-", '-'},                     // sub
  {"\\*", '*'},                     // times
  {"\\/", '/'},                     // divided
  {"0[xX][0-9a-fA-F]+",TK_HNUM},    // hexadecimal num
  {"\\$\\$?[a-zA-Z0-9]+",TK_REG},       // reg name
  {"[0-9]+",TK_DNUM},          // decimal num
  {"\\(", '('},                     // left bracket
  {"\\)", ')'},                     // right bracket
  {"&&", TK_AND},                    // and
  {"!=", TK_NEQ},                    // not equal
  {"==", TK_EQ},                    // equal
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
  int position = 0 ;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        if(substr_len > 32) {
          printf("A syntax error in expression, Long Parameters.\n");
          return false; 
        }

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len; 

        switch (rules[i].token_type) {
          case TK_NOTYPE: break; 
          case TK_DNUM:
            tokens[nr_token].type = rules[i].token_type;  
            memcpy(tokens[nr_token].str,substr_start,substr_len); 
            nr_token++;
            break;
          case TK_HNUM:
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str,substr_start,substr_len); 
            nr_token++;
            break; 
          case TK_REG: 
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str,substr_start+1,substr_len-1); 
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
  int op = -1;
  int flag = 0;
  int pred = 0;//predences
  for(int i=p;i<=q;i++) {
    switch (tokens[i].type) {
    case '(': flag++;  break;
    case ')': flag--;  break;
    case TK_AND :  if(flag == 0) { op = i;pred=4;} break;
    case TK_NEQ : case TK_EQ : if(flag == 0 && pred <= 3) {op = i;pred = 3;} break;
    case '+': case '-': if(flag == 0 && pred <= 2) {op = i; pred = 2;} break;
    case '*': case '/': if(flag == 0 && pred <= 1) {op = i; pred = 1;} break;
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
    /* Single token.*/
    switch (tokens[p].type) {
      case TK_DNUM: 
        return atoi(tokens[p].str); 
      case TK_HNUM:
        return strtoul(tokens[p].str,NULL,16);
      case TK_REG: 
        return isa_reg_str2val(tokens[p].str,NULL);
    }
  }
  else if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1);
  }
  else {
    int op = get_mainop(p,q);
    if(op != -1) {
      word_t val1 = eval(p, op - 1);
      word_t val2 = eval(op + 1, q);
      switch (tokens[op].type) {
        case '+': return val1 + val2;
        case '-': return val1 - val2;
        case '*': return val1 * val2;
        case '/': assert(val2 != 0);return val1 / val2;
        case TK_AND : return val1 && val2;
        case TK_EQ :return val1 == val2;
        case TK_NEQ :return val1 != val2;
        default: assert(0);
      }
    }else{
      if(tokens[p].type == DEREF) {
        vaddr_t addr = eval(p+1,q);
        return vaddr_read(addr,4);
      }else {
        assert(0);
      }
    }
  }
  return 0;
}

static bool check_expr(int p ,int q)  {
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    if(tokens[p].type ==TK_DNUM || tokens[p].type==TK_HNUM)
      return true;
    else if(tokens[p].type==TK_REG) {
      bool success = true;
      isa_reg_str2val(tokens[p].str,&success);
      return success; 
    }
    else return false;
  } 
  else if (check_parentheses(p, q)) {
    return check_expr(p + 1, q - 1);
  }
  else  {
    int op = get_mainop(p,q);
    if(op != -1) {
      bool val1 = check_expr(p, op - 1);
      bool val2 = check_expr(op + 1, q);
      return val1&&val2;
    }else{
      if(tokens[p].type == DEREF) {
        return check_expr(p+1,q);
      }else {
        assert(0);
      }
    }
  } 
}

word_t expr(char *e, bool *success) {
  memset(tokens,0,sizeof(tokens));
  if (!make_token(e)) {
    if(success!=NULL)*success = false;
    return -1;
  }

  int certenType[]  = {'(','+','-','*','/',TK_AND,TK_NEQ,TK_EQ,DEREF};
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*') {
      if(i == 0) tokens[i].type = DEREF;
      else {
        for(int j = 0;j<9 ;j++) {
          if(tokens[i-1].type == certenType[j] ) {
            tokens[i].type = DEREF; 
            break;
          }
        }
      }
    }
  }

  if(nr_token == 0 || !check_brak(0,nr_token-1) ||!check_expr(0,nr_token-1)) {
    printf("A syntax error in expression!\n");
    if(success!=NULL)*success = false;
    return -1;
  }  

  return eval(0, nr_token-1);

}


void test_expr() {
    FILE *fp = fopen("/home/will/NJUPA/ics2023/nemu/tools/gen-expr/wrong_input", "r");
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