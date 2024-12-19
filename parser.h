#ifndef PARSER_H
#define PARSER_H


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "util.h"
#include "types.h"



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
            LIST_APPEND(tokens, char*, strdup("("));
            inp++;
            break;
        case ')':
            LIST_APPEND(tokens, char*, strdup(")"));
            inp++;
            break;
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            inp++;
            break;
        default: {
            // Collect the token
            int len = 0;
            char buf[1024];  // Safe buffer for token collection

            // Collect characters until we hit a delimiter
            while (*inp && !strchr("() \t\n\r", *inp)) {
                buf[len++] = *inp++;
            }
            buf[len] = '\0';

            // Check if it's a number
            if (isdigit(buf[0]) || (buf[0] == '-' && isdigit(buf[1]))) {
                char* endptr;
                strtof(buf, &endptr);
                if (*endptr == '\0') {  // Valid number
                    LIST_APPEND(tokens, char*, strdup(buf));
                    continue;
                }
            }

            // Not a number, treat as symbol
            LIST_APPEND(tokens, char*, strdup(buf));
            break;
        }
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
