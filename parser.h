#ifndef PARSER_H
#define PARSER_H


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "util.h"

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
