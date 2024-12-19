#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"
#include "parser.h"
#include "types.h"

typedef struct CompilerContext {
  hashtable* functions;
  int label_counter;
  bool debug_mode;
  FILE* output;
} CompilerContext;

void compile_function(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset);
void compile_expr(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset);

#define PUSH(x) fprintf(ctx->output, "push %s\n", x)
#define POP(x) fprintf(ctx->output, "pop %s\n", x)
#define ADD(src, dst) fprintf(ctx->output, "add %s, %s\n", src, dst)
#define MUL(src) fprintf(ctx->output, "mul %s\n", src)
#define IMUL(src, dst) fprintf(ctx->output, "mul %s, %s\n", src, dst)
#define MOV(src, dst) fprintf(ctx->output, "movq %s, %s\n", src, dst)
#define MOVNUM(num, dst) fprintf(ctx->output, "movq $%d, %s\n", num, dst)

CompilerContext* init_compiler(FILE* output) {
  CompilerContext* ctx = malloc(sizeof(CompilerContext));
  ctx->functions = ht_new();
  ctx->label_counter = 0;
  ctx->debug_mode = true;
  ctx->output = output;
  return ctx;
}

void free_compiler(CompilerContext* ctx) {
  ht_free(ctx->functions);
  if (ctx->output != stdout) {
    fclose(ctx->output);
  }
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
  }
  else {
    int offset = 0;
    int total_offset = compute_offset(e);
    PUSH("%rbp");
    MOV("%rsp", "%rbp");
    fprintf(ctx->output, "sub $%d, %%rsp\n", total_offset);

    compile_expr(ctx, e, offsets, &offset);

    POP("%rax");
    MOV("%rbp", "%rsp");
    POP("%rbp");
  }

  if (ctx->debug_mode) {
    fprintf(stderr, "DEBUG: compile finished\n");
  }
  fprintf(ctx->output, "\n\n");
}

void compile_function(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset) {
  if (ctx->debug_mode) fprintf(stderr, "DEBUG: compile_function called\n");

  LispFunc* func = e->data.func;
  fprintf(ctx->output, "%s:\n", func->name);
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
  fprintf(ctx->output, "ret\n");
}

void compile_expr(CompilerContext* ctx, LispExpr* e, hashtable* offsets, int* offset) {
  if (ctx->debug_mode)
    fprintf(stderr, "DEBUG: compile_expr called with type %d\n", e->type);

  switch (e->type) {
    case NUMBER:
      if (ctx->debug_mode)
        fprintf(stderr, "DEBUG: compiling NUMBER %f\n", e->data.number);
      fprintf(ctx->output, "push $%d\n", (int) e->data.number);
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
      fprintf(ctx->output, "movq %d(%%rbp), %%rax\n", ofst);
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
          fprintf(ctx->output, "call %s\n", op->data.symbol);
          if (list->size > 1) {
            fprintf(ctx->output, "add $%d, %%rsp\n", (list->size - 1) * 8);
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
          fprintf(ctx->output, "test %%rax, %%rax\n");
          fprintf(ctx->output, "jz else_%d\n", current_label);

          compile_expr(ctx, if_body, offsets, offset);
          fprintf(ctx->output, "jmp endif_%d\n", current_label);

          fprintf(ctx->output, "else_%d:\n", current_label);
          compile_expr(ctx, else_body, offsets, offset);

          fprintf(ctx->output, "endif_%d:\n", current_label);
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
            fprintf(ctx->output, "mov %%rax, -%d(%%rbp)\n", *offset);
            ht_set(offsets, name, -*offset);
          }
          LispExpr* body = list->data[2];
          compile_expr(ctx, body, offsets, offset);
        }

        else {
          compile_expr(ctx, list->data[1], offsets, offset);
          compile_expr(ctx, list->data[2], offsets, offset);
          POP("%rbx");
          POP("%rax");

          if (strcmp(op->data.symbol, "+") == 0) {
            ADD("%rbx", "%rax");
          } else if (strcmp(op->data.symbol, "*") == 0) {
            MUL("%rbx");
          } else if (strcmp(op->data.symbol, "=") == 0) {
            MOV("$0", "%rdi");
            fprintf(ctx->output, "cmp %%rbx, %%rax\n");
            fprintf(ctx->output, "mov $1, %%rsi\n");
            fprintf(ctx->output, "cmovz %%rsi, %%rdi\n");
            MOV("%rdi", "%rax");
          } else if (strcmp(op->data.symbol, "<") == 0) {
            MOV("$0", "%rdi");
            fprintf(ctx->output, "cmp %%rbx, %%rax\n");
            fprintf(ctx->output, "mov $1, %%rsi\n");
            fprintf(ctx->output, "cmovl %%rsi, %%rdi\n");
            MOV("%rdi", "%rax");
          } else if (strcmp(op->data.symbol, "-") == 0) {
            fprintf(ctx->output, "sub %%rbx, %%rax\n");
          }
          PUSH("%rax");
        }
      }
      break;
    }
  }
}

int example() {
  CompilerContext* ctx = init_compiler(stdout);

  fprintf(ctx->output, ".section .text\n");
  fprintf(ctx->output, ".global main\n");

  char* fact_def = "(define (factorial n) (if (eq? n 0) 1 (* n (factorial (+ n -1)))))";
  char* fib_def = "(define (fib n) (if (eq? n 0) 0 (if (eq? n 1) 1 (+ (fib (+ n -1)) (fib (+ n -2))))))";

  compile(ctx, fact_def);
  compile(ctx, fib_def);

  fprintf(ctx->output, "scheme_entry:\n");
  compile(ctx, "(+ (fib 5) (factorial 6))");

  fprintf(ctx->output, "ret\n\n");

  free_compiler(ctx);
  return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.scm> [-o output]\n", argv[0]);
        return 1;
    }

    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Cannot open input file %s\n", argv[1]);
        return 1;
    }

    // Read entire file into string
    fseek(input, 0, SEEK_END);
    long fsize = ftell(input);
    fseek(input, 0, SEEK_SET);

    char* content = malloc(fsize + 1);
    fread(content, fsize, 1, input);
    content[fsize] = '\0';
    fclose(input);

    FILE* output = stdout;
    if (argc > 3 && strcmp(argv[2], "-o") == 0) {
        output = fopen(argv[3], "w");
        if (!output) {
            fprintf(stderr, "Cannot open output file %s\n", argv[3]);
            return 1;
        }
    }

    CompilerContext* ctx = init_compiler(output);
    fprintf(ctx->output, ".section .text\n.global scheme_entry\n");

    // Split input into separate expressions and compile each one
    char* expr = content;
    int paren_count = 0;
    int start = 0;
    int i;

    for (i = 0; expr[i] != '\0'; i++) {
        if (expr[i] == '(') paren_count++;
        else if (expr[i] == ')') {
            paren_count--;
            if (paren_count == 0) {
                // Found complete expression
                char saved = expr[i + 1];
                expr[i + 1] = '\0';
                compile(ctx, expr + start);
                expr[i + 1] = saved;
                start = i + 1;
            }
        }
    }

    free_compiler(ctx);
    free(content);
    return 0;
}
