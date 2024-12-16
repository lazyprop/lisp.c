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
  int offset = 0, ifcount = 0;

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

    case CONS:
      fprintf(stderr, "CONS\n");
      LispExpr* first = e->data.cons.first;
      LispExpr* second = e->data.cons.second;
      _compile(first);
      _compile(second);
      
      MOVNUM(32, "%rdi");
      printf("call malloc\n");
      POP("%rcx");
      POP("%rbx");
      MOV("%rbx", "(%rax)");
      MOV("%rcx", "8(%rax)");
      PUSH("%rax");
      break;
      
      
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

        else if (!strcmp(op->data.symbol, "if")) {
          assert(list->size == 4);
          LispExpr* cond = list->data[1];
          LispExpr* if_body = list->data[2];
          LispExpr* else_body = list->data[3];

          _compile(cond);
          POP("%rax");
          printf("test %rax, %rax\n");
          printf("jz else_%d\n", ifcount);
          _compile(if_body);
          printf("jmp else_%d_end\n", ifcount);

          printf("else_%d:\n", ifcount);
          _compile(else_body);
          printf("else_%d_end:\n", ifcount);
          ifcount++;
        }

        else if (!strcmp(op->data.symbol, "car")) {
          assert(list->size == 2);
          LispExpr* cons = list->data[1];
          _compile(cons);
          POP("%rbx");
          MOV("(%rbx)", "%rax");
          PUSH("%rax");
        }

        else if (!strcmp(op->data.symbol, "cdr")) {
          assert(list->size == 2);
          LispExpr* cons = list->data[1];
          _compile(cons);
          POP("%rbx");
          MOV("8(%rbx)", "%rax");
          PUSH("%rax");
        }

        else {
          _compile(list->data[1]);
          _compile(list->data[2]);
          POP("%rax");
          POP("%rbx");

          if (strcmp(op->data.symbol, "+") == 0) {
            ADD("%rbx", "%rax");
          } else if (strcmp(op->data.symbol, "*") == 0) {
            MUL("%rbx");
          } else if (strcmp(op->data.symbol, "eq?") == 0) {
            MOV("$0", "%rdi");
            printf("cmp %%rax, %%rbx\n");
            printf("mov $1, %%rsi\n");
            printf("cmovz %%rsi, %%rdi\n");
            MOV("%rdi", "%rax");
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

  
  POP("%rax");
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
  char* if_expr = "(let ((x 0)) (if (eq? x 1) 10 20))";
  char* cons_expr = "(car (cdr (cons 100 (cons 200 300))))";

  CharList* tokens = tokenize(cons_expr);

  LispExpr* e = parse(tokens);
  fprintf(stderr, "total offset: %d\n", compute_offset(e));

  print_expr(e);

  compile(e);

  printf("ret\n\n");
}
