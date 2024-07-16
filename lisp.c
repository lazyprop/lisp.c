#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"
#include "parser.h" // commented to avoid warnings


#define PUSH(x) printf("push %s\n", x);
#define POP(x) printf("pop %s\n", x);
#define ADD(src, dst) printf("add %s, %s\n", src, dst);
#define MUL(src) printf("mul %s\n", src);
#define IMUL(src, dst) printf("mul %s, %s\n", src, dst);
#define MOV(src, dst) printf("mov %s, %s\n", src, dst);
#define MOVNUM(num, dst) printf("mov $%d, %s\n", num, dst);


void add_num(int x, int y) {
  MOVNUM(x, "%rax");
  MOVNUM(y, "%rbx");
  ADD("%rax", "%rbx");
  PUSH("%rbx");
}


// generates code for expression e
// pushes resulting value to the stack
void compile(LispExpr* e) {
  //printf("compiling\n");
  //print_expr(e);
  switch (e->type) {
  case NUMBER:
    printf("push $%d\n", (int) e->data.number);
    return;
  case SYMBOL:
    return;
  case LIST:
    LispList* list = e->data.list;
    compile(list->data[1]);
    compile(list->data[2]);
    POP("%rax");
    POP("%rbx");
    char* op = list->data[0]->data.symbol;
    if (strcmp(op, "+") == 0) {
      ADD("%rbx", "%rax");
    } if (strcmp(op, "*") == 0) {
      MUL("%rbx");
    }
    PUSH("%rax");
  }
}

int main() {
  char* header =
    ".section .text\n"
    ".global main\n"
    "main:\n";

  printf("%s\n", header);


  char* inp = "(* (+ 7 (* 3 3)) (+ 3 4))";
  GENERIC_LIST(char*)* tokens = tokenize(inp);
  LispExpr* e = parse(tokens);

  compile(e);
  POP("%rax");

  printf("ret\n");
}
