#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"
#include "parser.h"

typedef struct CompilerContext {
  hashtable* functions;
  int label_counter;
  bool debug_mode;
} CompilerContext;

void compile_function(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset);
void compile_expr(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset);

#define PUSH(x) printf("push %s\n", x);
#define POP(x) printf("pop %s\n", x);
#define ADD(src, dst) printf("add %s, %s\n", src, dst);
#define MUL(src) printf("mul %s\n", src);
#define IMUL(src, dst) printf("mul %s, %s\n", src, dst);
#define MOV(src, dst) printf("movq %s, %s\n", src, dst);
#define MOVNUM(num, dst) printf("movq $%d, %s\n", num, dst);

CompilerContext* init_compiler(void) {
  CompilerContext* ctx = malloc(sizeof(CompilerContext));
  ctx->functions = ht_new();
  ctx->label_counter = 0;
  ctx->debug_mode = true;
  return ctx;
}

void free_compiler(CompilerContext* ctx) {
  ht_free(ctx->functions);
  free(ctx);
}

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

void compile(CompilerContext* ctx, char* expr) {
  static hashtable* offsets = NULL;
  if (!offsets) offsets = ht_new();

  CharList* tokens = tokenize(expr);
  LispExpr* e = parse(tokens);
  if (ctx->debug_mode) {
    fprintf(stderr, "Compiling expression:\n");
    print_expr(e);
  }

  if (e->type == FUNCTION) {
    ht_set(ctx->functions, e->data.func->name, (int) e);
    int offset = 0;
    compile_expr(ctx, e, offsets, &offset);
  } else {
    int offset = 0;
    int total_offset = compute_offset(e);
    PUSH("%rbp");
    MOV("%rsp", "%rbp");
    printf("sub $%d, %rsp\n", total_offset);

    compile_expr(ctx, e, offsets, &offset);

    POP("%rax");
    MOV("%rbp", "%rsp");
    POP("%rbp");
  }

  if (ctx->debug_mode) {
    fprintf(stderr, "DEBUG: compile finished\n");
  }
  printf("\n\n");
}

void compile_function(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset) {
  if (ctx->debug_mode) fprintf(stderr, "DEBUG: compile_function called\n");

  LispFunc* func = e->data.func;
  printf("%s:\n", func->name);
  PUSH("%rbp");
  MOV("%rsp", "%rbp");

  hashtable* local_offsets = ht_new();

  for (int i = 0; i < func->params->size; i++) {
    const char* param = func->params->data[i];
    ht_set(local_offsets, param, 16 + (i * 8));
  }

  compile_expr(ctx, func->body, local_offsets, offset);

  POP("%rax");
  MOV("%rbp", "%rsp");
  POP("%rbp");
  printf("ret\n");
}

void compile_expr(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset) {
  if (ctx->debug_mode)
    fprintf(stderr, "DEBUG: compile_expr called with type %d\n", e->type);

  switch (e->type) {
    case NUMBER:
      if (ctx->debug_mode)
        fprintf(stderr, "DEBUG: compiling NUMBER %f\n", e->data.number);
      printf("push $%d\n", (int) e->data.number);
      break;

    case SYMBOL: {
      if (ctx->debug_mode)
        fprintf(stderr, "DEBUG: compiling SYMBOL %s\n", e->data.symbol);
      const char* symb = e->data.symbol;
      ht_entry* entry = ht_get(offsets, symb);
      if (!entry) {
        fprintf(stderr, "ERROR: Symbol '%s' not found in offset table\n", symb);
        exit(1);
      }
      int ofst = entry->val;
      assert(ofst);
      printf("movq %d(%%rbp), %%rax\n", ofst);
      PUSH("%rax");
      break;
    }

    case FUNCTION:
      if (ctx->debug_mode)
        fprintf(stderr, "DEBUG: compiling FUNCTION\n");
      compile_function(ctx, e, offsets, offset);
      break;

    case LIST: {
      if (ctx->debug_mode)
        fprintf(stderr, "DEBUG: compiling LIST\n");
      LispList* list = e->data.list;
      LispExpr* op = list->data[0];

      if (op->type == SYMBOL) {
        if (ctx->debug_mode)
          fprintf(stderr, "DEBUG: list operator is %s\n", op->data.symbol);

        ht_entry* func_entry = ht_get(ctx->functions, op->data.symbol);
        if (func_entry) {
          for (int i = list->size-1; i >= 1; i--) {
            compile_expr(ctx, list->data[i], offsets, offset);
          }
          printf("call %s\n", op->data.symbol);
          if (list->size > 1) {
            printf("add $%d, %%rsp\n", (list->size - 1) * 8);
          }
          PUSH("%rax");
        }
        else if (!strcmp(op->data.symbol, "if")) {
          assert(list->size == 4);
          LispExpr* cond = list->data[1];
          LispExpr* if_body = list->data[2];
          LispExpr* else_body = list->data[3];

          int current_label = ctx->label_counter++;

          compile_expr(ctx, cond, offsets, offset);
          POP("%rax");
          printf("test %%rax, %%rax\n");
          printf("jz else_%d\n", current_label);

          compile_expr(ctx, if_body, offsets, offset);
          printf("jmp endif_%d\n", current_label);

          printf("else_%d:\n", current_label);
          compile_expr(ctx, else_body, offsets, offset);

          printf("endif_%d:\n", current_label);
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
            compile_expr(ctx, second, offsets, offset);
            POP("%rax");
            printf("mov %%rax, -%d(%%rbp)\n", *offset);
            ht_set(offsets, name, -*offset);
          }
          LispExpr* body = list->data[2];
          compile_expr(ctx, body, offsets, offset);
        }
        else {
          compile_expr(ctx, list->data[1], offsets, offset);
          compile_expr(ctx, list->data[2], offsets, offset);
          POP("%rax");
          POP("%rbx");

          if (strcmp(op->data.symbol, "+") == 0) {
            ADD("%rbx", "%rax");
          } else if (strcmp(op->data.symbol, "*") == 0) {
            MUL("%rbx");
          } else if (strcmp(op->data.symbol, "eq?") == 0) {
            MOV("$0", "%rdi");
            printf("cmp %%rbx, %%rax\n");
            printf("mov $1, %%rsi\n");
            printf("cmovz %%rsi, %%rdi\n");
            MOV("%rdi", "%rax");
          }

          PUSH("%rax");
        }
      }
      break;
    }
  }
}

int main() {
  CompilerContext* ctx = init_compiler();

  printf(".section .text\n");
  printf(".global scheme_entry\n");

  char* fact_def = "(define (factorial n) (if (eq? n 0) 1 (* n (factorial (+ n -1)))))";
  char* fib_def = "(define (fib n) (if (eq? n 0) 0 (if (eq? n 1) 1 (+ (fib (+ n -1)) (fib (+ n -2))))))";

  compile(ctx, fact_def);
  compile(ctx, fib_def);

  printf("scheme_entry:\n");
  compile(ctx, "(+ (fib 5) (factorial 6))");

  printf("ret\n\n");

  free_compiler(ctx);
  return 0;
}
