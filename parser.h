#ifndef PARSER_H
#define PARSER_H


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "util.h"

typedef enum { LIST, NUMBER, SYMBOL, CONS } ExprType;

struct LispExpr;

typedef GENERIC_LIST(struct LispExpr*) LispList;

typedef struct LispExpr {
  ExprType type;
  union {
    LispList* list;
    char* symbol;
    float number;
    struct { struct LispExpr* first; struct ListExpr* second } cons;
  } data;
} LispExpr;


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
  LIST_INIT(e->data.list);
  return e;
}


LispExpr* make_cons(LispExpr* first, LispExpr* second) {
  LispExpr* e = malloc(sizeof(LispExpr));
  e->type = CONS;
  e->data.cons.first = first;
  e->data.cons.second = second;
  return e;
}


GENERIC_LIST(char*)* tokenize(char* inp) {
  GENERIC_LIST(char*)* tokens = malloc(sizeof(GENERIC_LIST(char*)));
  LIST_INIT(tokens);
  while (*inp != '\0') {
    switch (*inp) {
    case '(':
      LIST_APPEND(tokens, "(");
      inp++;
      break;
    case ')':
      LIST_APPEND(tokens, ")");
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
      while (*inp != '\0' && !strchr("() \t\n\r", *inp)) {
        LIST_APPEND(&str, *inp);
        inp++;
      }
      char* tmp = malloc(str.size * sizeof(char));
      strncpy(tmp, str.data, str.size);
      LIST_APPEND(tokens, tmp);
    }
  }
  return tokens;
}

LispExpr* parse(GENERIC_LIST(char*)* tokens) {
  int idx = 0;
  LispExpr* _parse() {
    for (; idx < tokens->size; idx++) {
      char* tok = tokens->data[idx];
      if (strcmp(tok, "(") == 0) {
        if (!strcmp(tokens->data[idx+1], "cons")) {
          idx += 2;
          LispExpr* first = _parse();
          idx++;
          LispExpr* second = _parse();
          idx++;
          return make_cons(first, second);
        }

        LispExpr* e = make_list();
        LispList* list = e->data.list;
        for (idx++; idx < tokens->size; idx++) {
          LispExpr* tmp = _parse();
          if (!tmp) break;
          LIST_APPEND(list, tmp);
        }
        return e;
      }
      else if (strcmp(tok, ")") == 0) return NULL;
      else {
        int is_num = true;
        for (char* it = tok; *it != '\0'; it++) {
          if (!isdigit(*it)) {
            is_num = false;
            break;
          }
        }
        if (is_num) return make_number(atoi(tok));
        return make_symbol(tok);
      }
    }
  }
  return _parse();
}

#endif
