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
#define MOV(src, dst) printf("movq %s, %s\n", src, dst);
#define MOVNUM(num, dst) printf("movq $%d, %s\n", num, dst);


// compute total offset required for 'let' definitions for compiling e
int compute_offset(LispExpr* e) {
  if (e->type == LIST) {
    LispList* list = e->data.list;
    LispExpr* op = list->data[0];
    if (op->type == SYMBOL) {
      if (!strcmp(op->data.symbol, "let")) {
        assert(list->size == 3);
        assert(list->data[1]->type == LIST);
        LispList* assignments = list->data[1]->data.list;
        LispExpr* body = list->data[2];
        return (assignments->size * 8) + compute_offset(body);
      }
    }
    int offset = 0;
    for (int i = 0; i < list->size; i++) {
      offset += compute_offset(list->data[i]);
    }
    return offset;
  }
  return 0;
}

// generates code for top-level expression e
// pushes resulting value to the stack
void compile(LispExpr* e) {
  hashtable* offsets = ht_new();
  int offset = 0;

  void _compile(LispExpr* e) {
    switch (e->type) {
    case NUMBER:
      printf("push $%d\n", (int) e->data.number);
      break;

    case SYMBOL:
      const char* symb = e->data.symbol;
      int ofst = ht_get(offsets, symb)->val;
      assert(ofst);
      printf("movq -%d(%%rbp), %rax\n", ofst);
      PUSH("%rax");

      break;;
      
    case LIST:
      LispList* list = e->data.list;
      LispExpr* op = list->data[0];

      // handle special forms
      if (op->type == SYMBOL) {
        if (!strcmp(op->data.symbol, "let")) {
          assert(list->size == 3);
          assert(list->data[1]->type == LIST);
          LispList* assignments = list->data[1]->data.list;
          for (int i = 0; i < assignments->size; i++) {
            assert(assignments->data[i]->type == LIST);
            assert(assignments->data[i]->data.list->size == 2);
            LispExpr* first = assignments->data[i]->data.list->data[0];
            LispExpr* second = assignments->data[i]->data.list->data[1];
            assert(first->type == SYMBOL);
            const char* name = first->data.symbol;


            offset += 8;
            _compile(second);
            POP("%rax");
            printf("mov %rax, -%d(%rbp)\n", offset);
            ht_set(offsets, name, offset);
          }
          LispExpr* body = list->data[2];
          _compile(body);
        }


        else {
          _compile(list->data[1]);
          _compile(list->data[2]);
          POP("%rax");
          POP("%rbx");

          if (strcmp(op->data.symbol, "+") == 0) {
            ADD("%rbx", "%rax");
          } if (strcmp(op->data.symbol, "*") == 0) {
            MUL("%rbx");
          }
          PUSH("%rax");
        }
      }
    }
  }

  int total_offset = compute_offset(e);
  PUSH("%rbp");
  MOV("%rsp", "%rbp");
  printf("sub $%d, %rsp\n", total_offset);


  _compile(e);

  
  MOV("%rbp", "%rsp");
  POP("%rbp");


}


int main() {
  char* header =
    ".section .text\n"
    ".global scheme_entry\n"
    "scheme_entry:\n";
  

  printf("%s\n", header);


  char* arith_expr = "(* (+ 7 (* x 3)) (+ 3 4))";
  char* let_expr = "(let ((x 10) (y 7) (z 4)) (* (+ y (* x 3)) (+ 3 z))";
  GENERIC_LIST(char*)* tokens = tokenize(let_expr);

  LispExpr* e = parse(tokens);
  fprintf(stderr, "total offset: %d\n", compute_offset(e));

  compile(e);

  printf("ret\n\n");
}
