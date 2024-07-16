#ifndef UTIL_H
#define UTIL_H


#define GENERIC_LIST(t) struct { int size; int capacity; t* data;  }
#define LIST_DEFAULT(t) { 0, 32, malloc(32 * sizeof(t)) }
#define LIST_INIT(l) (l)->size = 0; (l)->capacity = 32; \
  (l)->data = malloc(32 * sizeof l->data);
#define LIST_APPEND(l, val)     \
  if ((l)->size == (l)->capacity) \
    (l)->data = reallocarray((l)->data, (l)->capacity *= 2, sizeof val); \
  (l)->data[(l)->size++] = val;
#define LIST_DESTROY(l) free((l)->data); free((l));


void test_list() {
  GENERIC_LIST(char) str = LIST_DEFAULT(char*);
  LIST_APPEND(&str, 'a');
  LIST_APPEND(&str, 'b');
  printf("%d\n", str.size);
  for (int i = 0; i < str.size; i++) {
    printf("%c ", str.data[i]);
  }
  printf("\n");
  LIST_APPEND(&str, 'c');
  printf("%d\n", str.capacity);
  printf("%d\n", str.size);
  for (int i = 0; i < str.size; i++) {
    printf("%c ", str.data[i]);
  }
  printf("\n");

  GENERIC_LIST(int) list = LIST_DEFAULT(int);
  LIST_APPEND(&list, 1);
  LIST_APPEND(&list, 2);
  LIST_APPEND(&list, 3);
  LIST_APPEND(&list, 4);
  LIST_APPEND(&list, 5);
  for (int i = 0; i < list.size; i++) {
    printf("%d ", list.data[i]);
  }
  printf("\n");
}
  
typedef enum { LIST, NUMBER, SYMBOL } ExprType;

struct LispExpr;

typedef GENERIC_LIST(struct LispExpr*) LispList;

typedef struct LispExpr {
  ExprType type;
  union {
    LispList* list;
    char* symbol;
    float number;
  } data;
} LispExpr;


void print_expr(LispExpr* e) {
  void _print(LispExpr* e, int level) {
    for (int i = 0; i < level; i++) {
      printf("  ");
    }
    switch (e->type) {
    case LIST:
      printf("<LIST %d>\n", e->data.list->size);
      int n = e->data.list->size;
      for (int j = 0; j < n; j++) {
        _print(e->data.list->data[j], level+1);
      }
      printf("\n");
      break;
    case SYMBOL:
      printf("<SYMBOL %s>\n", e->data.symbol);
      break;
    case NUMBER:
      printf("<NUMBER %.1f>\n", e->data.number);
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


#endif
