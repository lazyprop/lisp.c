#ifndef PARSER_H
#define PARSER_H


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "util.h"

typedef enum { LIST, NUMBER, SYMBOL, CONS, FUNCTION } ExprType;

struct LispExpr;

typedef GENERIC_LIST(struct LispExpr*) LispList;

struct LispFunc;

typedef struct LispExpr {
  ExprType type;
  union {
    LispList* list;
    char* symbol;
    float number;
    struct { struct LispExpr* first; struct LispExpr* second; } cons;
    struct LispFunc* func;
  } data;
} LispExpr;

typedef struct LispFunc {
  char* name;
  CharList* params;
  LispExpr* body;
} LispFunc;

void print_expr(LispExpr* e) {
  void _print(LispExpr* e, int level) {
    for (int i = 0; i < level; i++) {
      fprintf(stderr, "  ");
    }
    switch (e->type) {
    case LIST:
      fprintf(stderr, "<LIST %d>\n", e->data.list->size);
      int n = e->data.list->size;
      for (int j = 0; j < n; j++) {
        _print(e->data.list->data[j], level+1);
      }
      fprintf(stderr, "\n");
      break;
    case SYMBOL:
      fprintf(stderr, "<SYMBOL %s>\n", e->data.symbol);
      break;
    case NUMBER:
      fprintf(stderr, "<NUMBER %.1f>\n", e->data.number);
      break;
    case CONS:
      fprintf(stderr, "<CONS>\n");
      _print(e->data.cons.first, level+1);
      _print(e->data.cons.second, level+1);
      break;
    }
  }
  _print(e, 0);
}

LispExpr* make_symbol(char* val) {
  LispExpr* e = malloc(sizeof(LispExpr));
  e->type = SYMBOL;
  e->data.symbol = val;
  return e;
}

LispExpr* make_number(float val) {
  LispExpr* e = malloc(sizeof(LispExpr));
  e->type = NUMBER;
  e->data.number = val;
  return e;
}

LispExpr* make_list() {
  LispExpr* e = malloc(sizeof(LispExpr));
  e->type = LIST;
  e->data.list = malloc(sizeof(LispList));
  LIST_INIT(e->data.list, struct LispExpr*);
  return e;
}


LispExpr* make_cons(LispExpr* first, LispExpr* second) {
  LispExpr* e = malloc(sizeof(LispExpr));
  e->type = CONS;
  e->data.cons.first = first;
  e->data.cons.second = second;
  return e;
}

LispExpr* make_func(char* name, CharList* params, LispExpr* body) {
  LispExpr* e = malloc(sizeof(LispExpr));
  e->type = FUNCTION;
  e->data.func = malloc(sizeof(LispFunc));
  e->data.func->name = name;
  e->data.func->params = params;
  e->data.func->body = body;
  return e;
}

CharList* tokenize(char* inp) {
  CharList* tokens = malloc(sizeof(CharList));
  LIST_INIT(tokens, char*);

  while (*inp != '\0') {
    switch (*inp) {
    case '(':
      LIST_APPEND(tokens, char*, "(");
      inp++;
      break;
    case ')':
      LIST_APPEND(tokens, char*, ")");
      inp++;
      break;
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      inp++;
      break;
    default:
      GENERIC_LIST(char) str = LIST_DEFAULT(char);
      // Handle negative numbers by including '-' as part of the number
      if (*inp == '-' && isdigit(*(inp + 1))) {
        LIST_APPEND(&str, char, *inp);
        inp++;
      }
      while (*inp != '\0' && !strchr("() \t\n\r", *inp)) {
        LIST_APPEND(&str, char, *inp);
        inp++;
      }
      char* tmp = malloc(str.size + 1);  // +1 for null terminator
      strncpy(tmp, str.data, str.size);
      tmp[str.size] = '\0';  // Null terminate the string
      LIST_APPEND(tokens, char*, tmp);
    }
  }
  return tokens;
}


static LispExpr* parse_expr(CharList* tokens, int* idx);

LispExpr* parse(CharList* tokens) {
  int idx = 0;
  return parse_expr(tokens, &idx);
}

static LispExpr* parse_expr(CharList* tokens, int* idx) {
  for (; *idx < tokens->size; (*idx)++) {
    char* tok = tokens->data[*idx];
    if (strcmp(tok, "(") == 0) {
      if (*idx + 1 < tokens->size && strcmp(tokens->data[*idx + 1], "define") == 0) {
        (*idx) += 2;  // Skip past '(' and 'define'
        if (strcmp(tokens->data[*idx], "(") == 0) {
          (*idx)++;  // Skip past the opening paren of function name and params
          char* fname = tokens->data[(*idx)++];  // Get function name

          CharList* params = malloc(sizeof(CharList));
          LIST_INIT(params, char*);

          while (*idx < tokens->size && strcmp(tokens->data[*idx], ")") != 0) {
            LIST_APPEND(params, char*, tokens->data[(*idx)++]);
          }
          (*idx)++;  // Skip past closing paren of params

          LispExpr* body = parse_expr(tokens, idx);
          (*idx)++;  // Skip past closing paren of define

          return make_func(fname, params, body);
        }
      } else {
        LispExpr* e = make_list();
        LispList* list = e->data.list;
        for ((*idx)++; *idx < tokens->size; (*idx)++) {
          LispExpr* tmp = parse_expr(tokens, idx);
          if (!tmp) break;
          LIST_APPEND(list, struct LispExpr*, tmp);
        }
        return e;
      }
    }
    else if (strcmp(tok, ")") == 0) return NULL;
    else {
      // Modified number checking to handle negative numbers
      char* endptr;
      float num = strtof(tok, &endptr);
      if (*endptr == '\0' || (tok[0] == '-' && isdigit(tok[1]))) {
        return make_number(num);
      }
      return make_symbol(tok);
    }
  }
  return NULL;
}


#endif
