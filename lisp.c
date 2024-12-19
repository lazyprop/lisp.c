#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"
#include "parser.h"

void compile_function(LispExpr* e, hashtable* offsets, hashtable* functions, int* offset);
void compile_expr(LispExpr* e, hashtable* offsets, hashtable* functions, int* offset, int* label);

#define PUSH(x) printf("push %s\n", x);
#define POP(x) printf("pop %s\n", x);
#define ADD(src, dst) printf("add %s, %s\n", src, dst);
#define MUL(src) printf("mul %s\n", src);
#define IMUL(src, dst) printf("mul %s, %s\n", src, dst);
#define MOV(src, dst) printf("movq %s, %s\n", src, dst);
#define MOVNUM(num, dst) printf("movq $%d, %s\n", num, dst);

// compute total offset required for 'let' definitions for compiling e
int compute_offset(LispExpr* e) {
  fprintf(stderr, "DEBUG: compute_offset called\n");
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
  fprintf(stderr, "DEBUG: compute_offset finished\n");
  return 0;
}

void compile(LispExpr* e) {
    fprintf(stderr, "DEBUG: compile called\n");
    static hashtable* offsets = NULL;
    static hashtable* functions = NULL;

    if (!offsets) offsets = ht_new();
    if (!functions) functions = ht_new();

    if (e->type == FUNCTION) {
        ht_set(functions, e->data.func->name, (int) e);
        int offset = 0, label = 0;
        compile_expr(e, offsets, functions, &offset, &label);  // Just compile the function
    } else {
        // Only do stack frame setup for non-function expressions
        int offset = 0, label = 0;
        int total_offset = compute_offset(e);
        PUSH("%rbp");
        MOV("%rsp", "%rbp");
        printf("sub $%d, %rsp\n", total_offset);

        compile_expr(e, offsets, functions, &offset, &label);

        POP("%rax");
        MOV("%rbp", "%rsp");
        POP("%rbp");
    }
    fprintf(stderr, "DEBUG: compile finished\n");
}

void compile_function(LispExpr* e, hashtable* offsets, hashtable* functions, int* offset) {
    fprintf(stderr, "DEBUG: compile_function called\n");
    LispFunc* func = e->data.func;

    printf("%s:\n", func->name);
    PUSH("%rbp");
    MOV("%rsp", "%rbp");

    hashtable* local_offsets = ht_new();

    // Parameters are now at 16(%rbp) and 24(%rbp)
    for (int i = 0; i < func->params->size; i++) {
        const char* param = func->params->data[i];
        ht_set(local_offsets, param, 16 + (i * 8));
    }

    int label = 0;
    compile_expr(func->body, local_offsets, functions, offset, &label);

    // Get return value into %rax
    POP("%rax");

    MOV("%rbp", "%rsp");
    POP("%rbp");
    printf("ret\n");
}

void compile_expr(LispExpr* e, hashtable* offsets, hashtable* functions, int* offset, int* label) {
  fprintf(stderr, "DEBUG: compile_expr called with type %d\n", e->type);

  switch (e->type) {
  case NUMBER:
    fprintf(stderr, "DEBUG: compiling NUMBER %f\n", e->data.number);
    printf("push $%d\n", (int) e->data.number);
    fprintf(stderr, "DEBUG: NUMBER compilation finished\n");
    break;

  case SYMBOL:
    {
        fprintf(stderr, "DEBUG: compiling SYMBOL %s\n", e->data.symbol);
        const char* symb = e->data.symbol;
        ht_entry* entry = ht_get(offsets, symb);
        fprintf(stderr, "DEBUG: offset lookup result: %p\n", (void*)entry);
        if (!entry) {
          fprintf(stderr, "ERROR: Symbol '%s' not found in offset table\n", symb);
          exit(1);
        }
        int ofst = entry->val;
        assert(ofst);
        printf("movq %d(%%rbp), %%rax\n", ofst);  // Changed to use positive offset
        PUSH("%rax");
        fprintf(stderr, "DEBUG: SYMBOL compilation finished\n");
    }
    break;

  case FUNCTION:
    fprintf(stderr, "DEBUG: compiling FUNCTION\n");
    compile_function(e, offsets, functions, offset);
    fprintf(stderr, "DEBUG: FUNCTION compilation finished\n");
    break;

  case CONS:
    fprintf(stderr, "DEBUG: compiling CONS\n");
    {
      LispExpr* first = e->data.cons.first;
      LispExpr* second = e->data.cons.second;
      compile_expr(first, offsets, functions, offset, label);
      compile_expr(second, offsets, functions, offset, label);

      MOVNUM(32, "%rdi");
      printf("call malloc\n");
      POP("%rcx");
      POP("%rbx");
      MOV("%rbx", "(%rax)");
      MOV("%rcx", "8(%rax)");
      PUSH("%rax");
      fprintf(stderr, "DEBUG: CONS compilation finished\n");
    }
    break;

  case LIST:
    {
      fprintf(stderr, "DEBUG: compiling LIST\n");
      LispList* list = e->data.list;
      LispExpr* op = list->data[0];

      if (op->type == SYMBOL) {
        fprintf(stderr, "DEBUG: list operator is %s\n", op->data.symbol);
        ht_entry* func_entry = ht_get(functions, op->data.symbol);
        if (func_entry) {
          // Push arguments in reverse order
          for (int i = list->size-1; i >= 1; i--) {
            compile_expr(list->data[i], offsets, functions, offset, label);
          }
          printf("call %s\n", op->data.symbol);
          // Clean up arguments
          if (list->size > 1) {
            printf("add $%d, %%rsp\n", (list->size - 1) * 8);
          }
          PUSH("%rax");
          fprintf(stderr, "DEBUG: function call compilation finished\n");
        }

        else if (!strcmp(op->data.symbol, "let")) {
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

            *offset += 8;
            compile_expr(second, offsets, functions, offset, label);
            POP("%rax");
            printf("mov %%rax, -%d(%%rbp)\n", *offset);
            ht_set(offsets, name, *offset);
          }
          LispExpr* body = list->data[2];
          compile_expr(body, offsets, functions, offset, label);
          fprintf(stderr, "DEBUG: let compilation finished\n");
        }

        else if (!strcmp(op->data.symbol, "if")) {
            assert(list->size == 4);
            LispExpr* cond = list->data[1];
            LispExpr* if_body = list->data[2];
            LispExpr* else_body = list->data[3];

            int current_label = *label;
            (*label)++;

            compile_expr(cond, offsets, functions, offset, label);  // Pushes condition result
            POP("%rax");
            printf("test %%rax, %%rax\n");
            printf("jz else_%d\n", current_label);

            compile_expr(if_body, offsets, functions, offset, label);  // Pushes if-branch result
            printf("jmp endif_%d\n", current_label);

            printf("else_%d:\n", current_label);
            compile_expr(else_body, offsets, functions, offset, label);  // Pushes else-branch result

            printf("endif_%d:\n", current_label);
        }

        else if (!strcmp(op->data.symbol, "car")) {
          assert(list->size == 2);
          LispExpr* cons = list->data[1];
          compile_expr(cons, offsets, functions, offset, label);
          POP("%rbx");
          MOV("(%rbx)", "%rax");
          PUSH("%rax");
          fprintf(stderr, "DEBUG: car compilation finished\n");
        }

        else if (!strcmp(op->data.symbol, "cdr")) {
          assert(list->size == 2);
          LispExpr* cons = list->data[1];
          compile_expr(cons, offsets, functions, offset, label);
          POP("%rbx");
          MOV("8(%rbx)", "%rax");
          PUSH("%rax");
          fprintf(stderr, "DEBUG: cdr compilation finished\n");
        }

        else {
          compile_expr(list->data[1], offsets, functions, offset, label);
          compile_expr(list->data[2], offsets, functions, offset, label);
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
          fprintf(stderr, "DEBUG: operator compilation finished\n");
        }
      }
      fprintf(stderr, "DEBUG: LIST compilation finished\n");
    }
  }
  fprintf(stderr, "DEBUG: compile_expr finished\n");
}

int main() {
  char* header =
    ".section .text\n"
    ".global scheme_entry\n";

  printf("%s\n", header);

  char* arith_expr = "(* (+ 7 (* x 3)) (+ 3 4))";
  char* let_expr = "(let ((x 10) (y 7) (z 4)) (* (+ y (* x 3)) (+ 3 z))";
  char* if_expr = "(let ((x 0)) (if (eq? x 1) 10 20))";
  char* cons_expr = "(car (cdr (cons 100 (cons 200 300))))";
  char* func_def = "(define (factorial n) (if (eq? n 0) 1 (* n (factorial (+ n -1)))))";
  char* func_call = "(factorial 7)";

  CharList* tokens = tokenize(func_def);
  LispExpr* def = parse(tokens);
  fprintf(stderr, "Function definition:\n");
  print_expr(def);
  compile(def);

  printf("\n\n");

  tokens = tokenize(func_call);
  LispExpr* call = parse(tokens);
  fprintf(stderr, "\nFunction call:\n");
  print_expr(call);
  printf("scheme_entry:\n");
  compile(call);

  printf("ret\n\n");
}
