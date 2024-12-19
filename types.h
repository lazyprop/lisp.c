#ifndef TYPES_H
#define TYPES_H

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
#endif
